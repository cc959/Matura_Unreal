// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <vector>
#include <Engine/StreamableManager.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/ObjectLibrary.h"
#include "LevelManager.generated.h"


UCLASS()
class MATURA_UNREAL_API ALevelManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	bool key_pressed = false;

	FName current_level;
	
public:

	UObjectLibrary* object_library;
	FStreamableManager manager;
	TSharedPtr<FStreamableHandle> blubpointer;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	TArray<FName> sublevels;

	FVector2d viewport_size;

	int level = 0;
	int level_loaded = -1;


};