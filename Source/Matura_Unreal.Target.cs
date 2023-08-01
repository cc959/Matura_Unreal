// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Matura_UnrealTarget : TargetRules
{
	public Matura_UnrealTarget(TargetInfo Target) : base(Target)
	{
		bForceEnableExceptions = true;
		bOverrideBuildEnvironment = true;

		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
		ExtraModuleNames.Add("Matura_Unreal");
	}
}
