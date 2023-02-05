// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"

void UMyBlueprintFunctionLibrary::Pause(ASequencePlayer* player)
{
	if (player)
	player->Pause();
}
