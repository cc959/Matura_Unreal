// Copyright Epic Games, Inc. All Rights Reserved.

#include "Matura_UnrealGameMode.h"
#include "Matura_UnrealCharacter.h"

#include "CameraControl.h"
#include "TrackingCamera.h"

#include "UObject/ConstructorHelpers.h"

AMatura_UnrealGameMode::AMatura_UnrealGameMode()
{
	// set default pawn class to our Blueprinted character
	
	DefaultPawnClass = ATrackingCamera::StaticClass();
	PlayerControllerClass = ACameraControl::StaticClass();
}
