// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsSpawning : ModuleRules
{
	public DrsSpawning(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
