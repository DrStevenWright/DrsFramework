// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsBaseUtilities : ModuleRules
{
	public DrsBaseUtilities(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
