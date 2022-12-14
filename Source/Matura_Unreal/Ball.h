// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CameraManager.h"
#include "TrackingCamera.h"
#include "GameFramework/Actor.h"
#include "Ball.generated.h"

UCLASS()
class MATURA_UNREAL_API ABall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABall();
	FVector position = FVector(0, 0, 0);

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

	UPROPERTY(EditAnywhere, DisplayName="Displayed Ball size (m)")
	float display_size = 0.5;
	
	UPROPERTY(EditAnywhere, DisplayName="Ball size (m)")
	float ball_size = 0;

	UPROPERTY(EditAnywhere)
	TArray<ATrackingCamera*> tracking_cameras;
};
