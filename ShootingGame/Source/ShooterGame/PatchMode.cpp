// Fill out your copyright notice in the Description page of Project Settings.


#include "PatchMode.h"
#include "PatchManager.h"
#include "GameFramework/PlayerController.h"

APatchMode::APatchMode(const class FObjectInitializer& PCIP) : Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;

	this->DefaultPawnClass = nullptr;
	this->PlayerControllerClass = APlayerController::StaticClass();
}

void APatchMode::BeginPlay()
{
	Super::BeginPlay();

	PatchManager = UPatchManager::Get();
	check(PatchManager);

	if (PatchManager->IsRemoteManifestReady())
	{
		UE_LOG(LogTemp, Warning, TEXT("APatchMode::BeginPlay : RemoteManifest Ready"));
		StartInstall();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("APatchMode::BeginPlay : No RemoteManifest Download Manifest"));
		PatchManager->OnRemoteManifestDownloaded.AddUObject(this, &APatchMode::OnRemoteManifestReady);
		PatchManager->DownloadRemoteManifest();
	}
}

void APatchMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PatchManager->IsInstallingPatch())
	{
		BuildPatchServices::EBuildPatchState State = PatchManager->GetState();
		if (State <= BuildPatchServices::EBuildPatchState::Resuming)
		{

		}
		else if (State == BuildPatchServices::EBuildPatchState::Downloading)
		{

		}
		else if (State == BuildPatchServices::EBuildPatchState::BuildVerification)
		{

		}
	}
}

void APatchMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	PatchManager->Release();
	PatchManager = nullptr;
}

void APatchMode::OnRemoteManifestReady(bool bSucceed)
{
	if (bSucceed)
	{
		UE_LOG(LogTemp, Warning, TEXT("APatchMode::OnRemoteManifestReady : Download Manifest Successful"));
		StartInstall();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("APatchMode::OnRemoteManifestReady : Fail to Download Manifest"));
	}
}

void APatchMode::StartInstall()
{
	PatchManager->InstallCompleteDelegate.AddUObject(this, &APatchMode::OnInstallComplete);
	PatchManager->StartInstall();
}

void APatchMode::OnInstallComplete(bool bSucceed, EBuildPatchInstallError InErrorType)
{
	if (bSucceed)
	{
		PatchManager->MountPaks();
		UGameplayStatics::OpenLevel(GetWorld(), FName("ShooterEntry"));
	}
	else 
	{
		
	}
}

void APatchMode::LoadIntro()
{

}
