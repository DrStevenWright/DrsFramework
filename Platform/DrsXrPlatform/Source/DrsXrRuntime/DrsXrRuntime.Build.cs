// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsXrRuntime : ModuleRules
{
	public DrsXrRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Loads at EarliestPossible, before the engine: depend on Core only.
		PublicDependencyModuleNames.AddRange(new string[] { "Core" });
	}
}
