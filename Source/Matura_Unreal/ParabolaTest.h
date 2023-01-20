// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "CameraManager.h"


#include "ParabolaTest.generated.h"


UCLASS()
class MATURA_UNREAL_API AParabolaTest : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AParabolaTest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	CameraManager::ParabPath path;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool ShouldTickIfViewportsOnly() const;


	UPROPERTY(EditAnywhere)
	FVector center;
	
	UPROPERTY(EditAnywhere)
	double radius;
	
	UPROPERTY(EditAnywhere)
	TArray<AActor*> targets;

};
