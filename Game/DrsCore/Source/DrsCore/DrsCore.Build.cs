// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsCore : ModuleRules
{
	public DrsCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DrsBase" });

		PrivateDependencyModuleNames.AddRange(new string[] { "HeadMountedDisplay", "EnhancedInput", "DrsInput", "DrsXrPlatform", "DrsXrGame" });
	}
}
