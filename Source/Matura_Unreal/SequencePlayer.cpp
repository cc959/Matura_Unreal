// Fill out your copyright notice in the Description page of Project Settings.


#include "SequencePlayer.h"

#include <EngineUtils.h>
#include <Kismet/GameplayStatics.h>


// Sets default values
ASequencePlayer::ASequencePlayer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
}

// Called when the game starts or when spawned
void ASequencePlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASequencePlayer::Pause()
{
	if (player_object && player_object->GetSequencePlayer() && player_object->GetSequencePlayer()->IsPlaying())
		player_object->GetSequencePlayer()->Pause();
}

bool ASequencePlayer::MoveForward()
{
	if (player_object && player_object->GetSequencePlayer())
	{
		auto player = player_object->GetSequencePlayer();

		int32 current_frame = player->GetCurrentTime().Time.FrameNumber.Value;

		if (current_frame < 0)
			player->SetPlaybackPosition({FFrameTime(0), {}}), player->Pause();

		if (abs(player->GetCurrentTime().Time.AsDecimal() - player->GetEndTime().Time.AsDecimal()) < 0.1)
		{
			return true;
		}
				
		player->SetPlaybackPosition({FFrameTime(current_frame + 1), {}});
		player->SetPlayRate(1);
		player->Play();
	}
	else
	{
		return true;
	}

	return false;
}

bool ASequencePlayer::MoveBackward()
{
	if (player_object && player_object->GetSequencePlayer())
	{
		auto player = player_object->GetSequencePlayer();

		int32 current_frame = player->GetCurrentTime().Time.FrameNumber.Value;

		if (current_frame < 0)
			player->SetPlaybackPosition({FFrameTime(0), {}}), player->Pause();

		if (player->GetCurrentTime().Time == player->GetStartTime().Time)
		{
			return true;
		}
				
		player->SetPlaybackPosition({FFrameTime(current_frame - 1), {}});
		player->SetPlayRate(-1);
		player->Play();
	}
	else
	{
		return true;
	}

	return false;
}

// Called every frame
void ASequencePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (player_object && player_object->GetSequencePlayer())
	{
		auto player = player_object->GetSequencePlayer();

		int32 current_frame = player->GetCurrentTime().Time.FrameNumber.Value;

		if (current_frame < 0)
			player->SetPlaybackPosition({FFrameTime(0), {}}), player->Pause();
	}
}

