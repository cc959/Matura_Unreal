// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParabPath.h"
#include "CameraManager.h"
#include "TrackingCamera.h"
#include "GameFramework/Actor.h"

#include "EventPasser.h"

#include "Ball.generated.h"

UCLASS()
class MATURA_UNREAL_API ABall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABall();
	FVector position = FVector(0, 0, 0);
	FVector overridden_position = FVector(0, 0, 0);
	EventPasser<ParabPath> tracking_path = {false};
	bool started = false;
	bool position_overridden = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UStaticMeshComponent* ball_mesh;

	class CameraManager* manager;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginDestroy() override;

	UPROPERTY(EditAnywhere, DisplayName="Displayed Ball size")
	double display_size = 0.5;
	
	UPROPERTY(EditAnywhere, DisplayName="Ball size")
	double ball_size = 0;

	UPROPERTY(EditAnywhere, DisplayName="Gravitational Constant g")
	double g = -9810;

	UPROPERTY(EditAnywhere, DisplayName="Save paths to file")
	bool save_paths = false;
	
	UPROPERTY(EditAnywhere)
	bool autodetect_cameras = true;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition="!autodetect_cameras", EditConditionHides))
	TArray<ATrackingCamera*> tracking_cameras;
};
