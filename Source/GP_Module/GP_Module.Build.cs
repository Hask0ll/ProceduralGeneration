// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GP_Module : ModuleRules
{
	public GP_Module(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent", "FastNoiseGenerator", "FastNoise" });

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
