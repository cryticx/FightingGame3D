// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FightingGame3D : ModuleRules
{
	public FightingGame3D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
