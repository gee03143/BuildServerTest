// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IBuildManifest.h"
#include "Interfaces/IBuildInstaller.h"
#include "PatchMode.generated.h"

UCLASS(config=Engine)
class APatchMode : public AGameModeBase
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	void OnRemoteManifestReady(bool bSucceed);
	void StartInstall();
	void OnInstallComplete(bool bSucceed, EBuildPatchInstallError InErrorType);

protected:
	void LoadIntro();

protected:
	class UPatchManager* PatchManager;

};
