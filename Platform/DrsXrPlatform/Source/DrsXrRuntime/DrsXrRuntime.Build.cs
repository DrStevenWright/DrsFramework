// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsXrRuntime : ModuleRules
{
	public DrsXrRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Loads at EarliestPossible, before the engine: Core, DrsBase for DRS_LOG (safe this early, see Base/CLAUDE.md),
		// and DrsXrArbiter for the plugin override and the shared runtime state. No UObjects are used here.
		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateDependencyModuleNames.AddRange(new string[] { "DrsBase", "DrsXrArbiter" });
	}
}
