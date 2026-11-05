// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsGraphicsUtilities : ModuleRules
{
	public DrsGraphicsUtilities(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "DrsGlobals" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
