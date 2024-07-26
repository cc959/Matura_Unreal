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
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Matura_Unreal");
	}
}
