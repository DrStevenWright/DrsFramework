// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsBase : ModuleRules
{
	public DrsBase(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Only for the EControllerHand values in DrsHand.cpp. Nothing from InputCore is linked, so DrsBase keeps its early-load
		// independence.
		PrivateIncludePathModuleNames.Add("InputCore");
	}
}
