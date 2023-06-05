// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrackingCamera.h"
#include "GameFramework/PlayerController.h"
#include "CameraControl.generated.h"

/**
 * 
 */
UCLASS()
class MATURA_UNREAL_API ACameraControl : public APlayerController
{
	GENERATED_BODY()
public:

	ACameraControl();

	virtual void Tick(float DeltaTime) override;

	ATrackingCamera* selected;

	int corner;
};
