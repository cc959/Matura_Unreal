// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelManager.h"

#include <Kismet/GameplayStatics.h>

// Sets default values
ALevelManager::ALevelManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ALevelManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALevelManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::PageDown))
	{
		if (key_pressed == false)
		{
			level++;
		}
		key_pressed = true;
	}
	else if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::PageUp))
	{
		if (key_pressed == false)
		{
			level--;
		}
		key_pressed = true;
	}
	else
	{
		key_pressed = false;
	}
	
	if (level != level_loaded)
	{
		if (level >= 0 && level < sublevels.Num() && !sublevels[level].IsNone())
		{
			// for some reason can't load and unload a level on the same frame, or perhaps I did it incorrectly
			if (current_level != "")
			{
				UGameplayStatics::UnloadStreamLevel(this, current_level, FLatentActionInfo(), true);

				current_level = "";
			} else
			{
				level_loaded = level;
				UE_LOG(LogTemp, Display, TEXT("Loading level number %d: %s, current level: %s"), level, *sublevels[level].ToString(), *current_level.ToString());
			
				UGameplayStatics::LoadStreamLevel(this, sublevels[level], true, true, FLatentActionInfo());
				current_level = sublevels[level];
			}
			
		} else
		{
			UE_LOG(LogTemp, Error, TEXT("Error loading level number %d"), level);
		}
	}
	
}

