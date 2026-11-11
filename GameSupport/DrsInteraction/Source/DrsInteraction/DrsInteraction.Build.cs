// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsInteraction : ModuleRules
{
	public DrsInteraction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
