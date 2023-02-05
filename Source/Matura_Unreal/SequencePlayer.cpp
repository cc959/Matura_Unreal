// Fill out your copyright notice in the Description page of Project Settings.


#include "SequencePlayer.h"

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
		
		if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::Right))
		{
			if (!key_pressed)
			{
				player->SetPlaybackPosition({FFrameTime(current_frame + 1), {}});
				player->SetPlayRate(1);
				player->Play();
	
				key_pressed = true;
			}
		} else if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::Left))
		{
			if (!key_pressed)
			{
				player->SetPlaybackPosition({FFrameTime(current_frame - 1), {}});
				player->SetPlayRate(-1);
				player->Play();
				key_pressed = true;
			}
		} else
		{
			key_pressed = false;
		}
	}
}

