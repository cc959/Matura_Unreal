// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "RobotArm.generated.h"

UCLASS()
class MATURA_UNREAL_API ARobotArm : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARobotArm();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdateRotations();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *base_component;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *lower_arm_component;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *upper_arm_component;

	UPROPERTY(EditAnywhere, Category = Rotation)
	float baseRotation;

	UPROPERTY(EditAnywhere, Category = Rotation)
	float lowerArmRotation;

	UPROPERTY(EditAnywhere, Category = Rotation)
	float upperArmRotation;
};
