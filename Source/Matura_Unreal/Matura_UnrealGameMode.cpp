// Copyright Epic Games, Inc. All Rights Reserved.

#include "Matura_UnrealGameMode.h"
#include "Matura_UnrealCharacter.h"

#include "CameraControl.h"
#include "FlyCharacter.h"
#include "TrackingCamera.h"

#include "UObject/ConstructorHelpers.h"

AMatura_UnrealGameMode::AMatura_UnrealGameMode()
{
	// set default pawn class to our Blueprinted character
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Spectator"));
	// static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Gadme/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	// if (PlayerPawnBPClass.Class != NULL)
	// 	DefaultPawnClass = PlayerPawnBPClass.Class;

	DefaultPawnClass = AFlyCharacter::StaticClass();
	
	PlayerControllerClass = ACameraControl::StaticClass();
}
