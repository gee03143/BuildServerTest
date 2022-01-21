// Fill out your copyright notice in the Description page of Project Settings.


#include "PatchManager.h"
#include "HttpModule.h"
#include "Interfaces/IBuildPatchServicesModule.h"
#include "IPlatformFilePak.h"

DEFINE_LOG_CATEGORY_STATIC(LogPatch, Log, All);


UPatchManager* UPatchManager::Singleton = nullptr;

void UPatchManager::BeginDestroy()
{
	Super::BeginDestroy();

	if (Installer.IsValid())
	{
		Installer->CancelInstall();
		Installer.Reset();
	}
}

UPatchManager* UPatchManager::Get()
{
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UPatchManager>();
		Singleton->Init();
		Singleton->AddToRoot();
	}
	check(Singleton);

	return Singleton;
}

void UPatchManager::Release()
{
	if (Singleton)
	{
		Singleton->RemoveFromRoot();
		Singleton = nullptr;
	}
}

bool UPatchManager::InstalledManifestExists()
{
	return InstalledManifest.IsValid();
}

bool UPatchManager::IsRemoteManifestReady()
{
	return RemoteManifest.IsValid();
}

void UPatchManager::DownloadRemoteManifest()
{
	UE_LOG(LogTemp, Warning, TEXT("UPatchManager::DownloadRemoteManifest : TryDownload Manifest"));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(*RemoteManifestURL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPatchManager::OnRemoteManifestReady);
	HttpRequest->ProcessRequest();
}

int64 UPatchManager::GetRequiredDownloadSize()
{
	if (RemoteManifest.IsValid() == false)
	{
		return 0;
	}
	else
	{
		TSet<FString> Tags;
		if (InstalledManifest.IsValid())
		{
			return RemoteManifest->GetDeltaDownloadSize(Tags, InstalledManifest.ToSharedRef());
		}
		else
		{
			return RemoteManifest->GetDownloadSize();
		}
	}
}

void UPatchManager::VerifyData()
{
	check(Installer.IsValid() == false);
	check(InstalledManifest.IsValid());

	TArray<BuildPatchServices::FInstallerAction> InstallerActions;
	InstallerActions.Add(BuildPatchServices::FInstallerAction::MakeRepair(InstalledManifest.ToSharedRef()));

	BuildPatchServices::FBuildInstallerConfiguration Configuration(MoveTemp(InstallerActions));
	Configuration.StagingDirectory = StageDir;
	Configuration.CloudDirectories = { CloudURL };
	Configuration.InstallDirectory = FullInstallDir;

	Configuration.VerifyMode = BuildPatchServices::EVerifyMode::ShaVerifyAllFiles;

	Installer = BuildPatchServices->CreateBuildInstaller(Configuration, FBuildPatchInstallerDelegate::CreateUObject(this, &UPatchManager::OnVerifyComplete));
	Installer->StartInstallation();
}

void UPatchManager::CancelVerify()
{
	if (Installer.IsValid() == false)
	{
		return;
	}

	Installer->CancelInstall();
}

bool UPatchManager::IsVerifyingData()
{
	return Installer.IsValid();
}

bool UPatchManager::VerifyDataSimplified()
{
	if (InstalledManifest.IsValid() == false)
	{
		return false;
	}

	FullInstallDir = FPaths::ProjectPersistentDownloadDir() / TEXT("Patch");
	TArray<FString> InstalledFileNames = InstalledManifest->GetBuildFileList();
	for (const FString& FileName : InstalledFileNames)
	{
		if (FPaths::GetExtension(FileName) == TEXT("pak"))
		{
			FString PakFullName = FullInstallDir / FileName;
			if (IFileManager::Get().FileSize(*PakFullName) != InstalledManifest->GetFileSize(FileName))
			{
				return false;
			}
		}
	}

	return true;
}

float UPatchManager::GetVerifyProgress()
{
	if (Installer.IsValid())
	{
		return Installer->GetStateProgress(BuildPatchServices::EBuildPatchState::BuildVerification);
	}
	else
	{
		return 0.f;
	}
}

void UPatchManager::StartInstall()
{
	StartBuildPatchService(CloudURL);
}

bool UPatchManager::IsInstallingPatch()
{
	return Installer.IsValid();
}

BuildPatchServices::EBuildPatchState UPatchManager::GetState()
{
	if (Installer.IsValid())
	{
		return Installer->GetState();
	}
	else
	{
		return BuildPatchServices::EBuildPatchState::NUM_PROGRESS_STATES;
	}
}

float UPatchManager::GetStateProgress(BuildPatchServices::EBuildPatchState InState)
{
	if (Installer.IsValid())
	{
		return Installer->GetStateProgress(InState);
	}
	else
	{
		return 0;
	}
}

int64 UPatchManager::GetTotalRequiredSize()
{
	if (Installer.IsValid())
	{
		return Installer->GetTotalDownloadRequired() + Installer->GetTrimmedByteSize();
	}
	else
	{
		return 0;
	}
}

int64 UPatchManager::GetTotalDownloaded()
{
	if (Installer.IsValid())
	{
		return Installer->GetTotalDownloaded() + Installer->GetTrimmedByteSize();
	}
	else
	{
		return 0;
	}
}

double UPatchManager::GetDownloadSpeed()
{
	if (Installer.IsValid())
	{
		return Installer->GetDownloadSpeed();
	}
	else
	{
		return 0;
	}
}

bool UPatchManager::MountPaks()
{
/*
#if WITH_EDITOR
	return true;
#else
*/
	FPakPlatformFile* PakFileManager = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile")));
	if (PakFileManager == nullptr)
	{
		UE_LOG(LogPatch, Log, TEXT("PakFileManager is null"));
		return false;
	}

	uint32 PakOrder = 1;
	if (InstalledManifest.IsValid())
	{
		TArray<FString> InstalledFileNames = InstalledManifest->GetBuildFileList();
		for (const FString& FileName : InstalledFileNames)
		{
			if (FPaths::GetExtension(FileName) == TEXT("pak"))
			{
				FString PakFullName = FullInstallDir / FileName;
				if (PakFileManager->Mount(*PakFullName, PakOrder))
				{
					UE_LOG(LogPatch, Log, TEXT("Mounted = %s, Order = %d"), *PakFullName, PakOrder);
				}
				else
				{
					UE_LOG(LogPatch, Error, TEXT("Failed to mount pak = %s"), *PakFullName);
					return false;
				}
			}
		}
		return true;
	}
	else
	{
		UE_LOG(LogPatch, Error, TEXT("No installed manifest, failed to mount"));
		return false;
	}
}

void UPatchManager::Init()
{
	FullInstallDir = FPaths::ProjectPersistentDownloadDir() / TEXT("Patch");
	StageDir = FPaths::ProjectPersistentDownloadDir() / TEXT("PatchString");
	TempChunkDir = FullInstallDir / TEXT("TempChunks");
	BuildPatchServices = &FModuleManager::LoadModuleChecked<IBuildPatchServicesModule>(TEXT("BuildPatchServices"));

	RemoteManifestURL = FString(TEXT("http://133.186.131.38:18080/ShootergameDist/Shootergame.manifest"));
	CloudURL = FString(TEXT("http://133.186.131.38:18080/ShootergameDist/"));

	UE_LOG(LogTemp, Warning, TEXT("RemoteManifestURL : %s"), *RemoteManifestURL);
	UE_LOG(LogTemp, Warning, TEXT("CloudURL : %s"), *CloudURL);

	FindInstalledManifest();
}

void UPatchManager::FindInstalledManifest()
{
	if (bSearchFlag)
	{
		return;
	}

	TArray<FString> InstalledManifestNames;
	IFileManager::Get().FindFiles(InstalledManifestNames, *(FullInstallDir / TEXT("*.manifest")), true, false);

	UE_LOG(LogTemp, Warning, TEXT("FindInstalledManifest: InstalledManifestNames.Num() : %d"), InstalledManifestNames.Num());

	if (InstalledManifestNames.Num())
	{
		if (InstalledManifestNames.Num() > 1)
		{
			UE_LOG(LogPatch, Warning, TEXT("More than one manifest files exists in install directory."));
		}

		const FString& InstalledManifestName = InstalledManifestNames[0];
		UE_LOG(LogTemp, Warning, TEXT("FindInstalledManifest: InstalledManifestNames.Name : %s"), *InstalledManifestName);

		InstalledManifest = BuildPatchServices->LoadManifestFromFile(FullInstallDir / InstalledManifestName);
		UE_LOG(LogPatch, Log, TEXT("Installed manifest found : %s"), *InstalledManifestName);
	}

	bSearchFlag = true;
}

void UPatchManager::StartBuildPatchService(const FString& InCloudDir)
{
	check(Installer.IsValid() == false);

	if (IsRemoteManifestReady() == false)
	{
		return;
	}

	TArray<BuildPatchServices::FInstallerAction> InstallerActions;
	InstallerActions.Add(BuildPatchServices::FInstallerAction::MakeInstallOrUpdate(InstalledManifest, RemoteManifest.ToSharedRef()));

	BuildPatchServices::FBuildInstallerConfiguration Configuration(MoveTemp(InstallerActions));
	Configuration.StagingDirectory = StageDir;
	Configuration.CloudDirectories = { InCloudDir };
	Configuration.InstallDirectory = FullInstallDir;

	Installer = BuildPatchServices->CreateBuildInstaller(Configuration, FBuildPatchInstallerDelegate::CreateUObject(this, &UPatchManager::OnInstallComplete));
	Installer->StartInstallation();
}

void UPatchManager::OnRemoteManifestReady(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded)
{
	if (!bSucceeded || !Response.IsValid())
	{
		UE_LOG(LogPatch, Error, TEXT("Failed to download manifest."));
		OnRemoteManifestDownloaded.Broadcast(false);
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	if (!EHttpResponseCodes::IsOk(ResponseCode))
	{
		UE_LOG(LogPatch, Error, TEXT("Failed to download manifest. ResponseCode = %d, ResponseMsg = %s"), ResponseCode, *Response->GetContentAsString());
		OnRemoteManifestDownloaded.Broadcast(false);
		return;
	}

	RemoteManifest = BuildPatchServices->MakeManifestFromData(Response->GetContent());
	if (!RemoteManifest.IsValid())
	{
		UE_LOG(LogPatch, Error, TEXT("Failed to create manifest."));
		OnRemoteManifestDownloaded.Broadcast(false);
		return;
	}
	OnRemoteManifestDownloaded.Broadcast(true);
}

void UPatchManager::OnVerifyComplete(const IBuildInstallerRef& InInstaller)
{
	check(Installer.Get() == &InInstaller.Get());

	VerifyCompleteDelegate.Broadcast(InInstaller->CompletedSuccessfully());
	Installer.Reset();
}

void UPatchManager::OnInstallComplete(const IBuildInstallerRef& InInstaller)
{
	check(Installer.Get() == &InInstaller.Get());

	if (Installer->CompletedSuccessfully())
	{
		FString ManifestFullFileName = FullInstallDir / TEXT("latest.manifest");
		
		if (!BuildPatchServices->SaveManifestToFile(ManifestFullFileName, RemoteManifest.ToSharedRef()))
		{
			UE_LOG(LogPatch, Error, TEXT("Failed to save manifest to disk. %s"), *ManifestFullFileName);
		}

		InstalledManifest = RemoteManifest;

		FString NewVersionStr = FString::Printf(TEXT("%d.%d.%d"), 0, 0, 0);
	}
	else
	{
		UE_LOG(LogPatch, Error, TEXT("Installation Failed. Code %d Msg: %s"),
			(int32)Installer->GetErrorType(),
			*Installer->GetErrorText().ToString());
	}

	InstallCompleteDelegate.Broadcast(Installer->CompletedSuccessfully(), Installer->GetErrorType());

	Installer.Reset();
}
