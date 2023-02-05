// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <LevelSequencePlayer.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelSequenceActor.h"
#include "SequencePlayer.generated.h"

UCLASS(Blueprintable)
class MATURA_UNREAL_API ASequencePlayer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASequencePlayer();

	//ULevelSequencePlayer* player;
	bool key_pressed = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	ALevelSequenceActor* player_object;

	void Pause();

};
