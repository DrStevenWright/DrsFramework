// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsUserInterface : ModuleRules
{
	public DrsUserInterface(ReadOnlyTargetRules Target) : base(Target)
	{
		// SWTODO - reorganise plugin.
		
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange([
			"UMG"
		]);
		
		PrivateDependencyModuleNames.AddRange([
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"EnhancedInput",
			"InputCore",
			"Slate",
			"SlateCore",
			
			"DrsBase",
			"DrsInputSystem"
		]);
	}
}
