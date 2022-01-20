// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IBuildManifest.h"
#include "Interfaces/IBuildInstaller.h"
#include "PatchManager.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRemoteManifestDownloaded, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVerifyComplete, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInstallComplete, bool, EBuildPatchInstallError);

UCLASS()
class UPatchManager : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void BeginDestroy() override;

public:
	static UPatchManager* Get();
	static void Release();

public:
	bool InstalledManifestExists();
	bool IsRemoteManifestReady();
	void DownloadRemoteManifest();

	int64 GetRequiredDownloadSize();

	void VerifyData();
	void CancelVerify();
	bool IsVerifyingData();
	float GetVerifyProgress();

	bool VerifyDataSimplified();

	void StartInstall();
	bool IsInstallingPatch();
	BuildPatchServices::EBuildPatchState GetState();
	float GetStateProgress(BuildPatchServices::EBuildPatchState InState);
	int64 GetTotalRequiredSize();
	int64 GetTotalDownloaded();
	double GetDownloadSpeed();

	bool MountPaks();

protected:
	void Init();
	void FindInstalledManifest();
	void StartBuildPatchService(const FString& InCloudDir);
	void OnRemoteManifestReady(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded);
	void OnVerifyComplete(const IBuildInstallerRef& InInstaller);
	void OnInstallComplete(const IBuildInstallerRef& InInstaller);

private:
	class IBuildPatchServicesModule* BuildPatchServices;

	IBuildManifestPtr InstalledManifest;
	IBuildManifestPtr RemoteManifest;
	IBuildInstallerPtr Installer;

	bool bSearchFlag;

	FString RemoteManifestURL;
	FString CloudURL;
	FString FullInstallDir;
	FString StageDir;
	FString TempChunkDir;

	TSharedPtr<class IBGChunkDownloader> BGChunkDownloader;
	TArray<FString> ChunksToDownload;
	int64 TotalRequiredChunkSize;
	int64 TotalChunkSizeToDownload;
	int64 DownloadedChunkSize;
	bool bChunkChecked;

public:
	FOnRemoteManifestDownloaded OnRemoteManifestDownloaded;
	FOnVerifyComplete VerifyCompleteDelegate;
	FOnInstallComplete InstallCompleteDelegate;

private:
	static UPatchManager* Singleton;
};
