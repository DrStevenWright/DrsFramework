// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsXrPlatformEditor : ModuleRules
{
	public DrsXrPlatformEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "DeveloperSettings", "DrsXrArbiter" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Engine" });
	}
}
