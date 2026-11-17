// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsBaseTests : ModuleRules
{
	public DrsBaseTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "DrsBase", "DrsBaseBlueprint" });

		// For the EControllerHand values used by the hand tests.
		PrivateIncludePathModuleNames.Add("InputCore");
	}
}
