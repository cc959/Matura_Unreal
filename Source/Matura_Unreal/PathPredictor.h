// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParabolaPredictor.h"
#include "GameFramework/Actor.h"
#include "PathPredictor.generated.h"

UCLASS()
class MATURA_UNREAL_API APathPredictor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APathPredictor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	ParabolaPredictor predictor;

	UPROPERTY(EditAnywhere, Category=Visualization)
	int num_steps;

	UPROPERTY(EditAnywhere, Category=Visualization)
	float start_time;

	UPROPERTY(EditAnywhere, Category=Visualization)
	float end_time;
};
