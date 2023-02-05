// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Matura_Unreal : ModuleRules
{
	public Matura_Unreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "OpenCV", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "UMG", "ImagePlate", "Slate", "SlateCore", "LevelSequence", "MovieScene" });
	}
}
