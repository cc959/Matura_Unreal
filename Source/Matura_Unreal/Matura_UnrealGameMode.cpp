// Copyright Epic Games, Inc. All Rights Reserved.

#include "Matura_UnrealGameMode.h"
#include "Matura_UnrealCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMatura_UnrealGameMode::AMatura_UnrealGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		}
}
