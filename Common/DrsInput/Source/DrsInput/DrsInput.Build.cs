// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsInput : ModuleRules
{
	public DrsInput(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
		[
			"DeveloperSettings", 
			"EnhancedInput",
			"DrsBase", // SWTODO - can this be private?
		]);
		
		PrivateDependencyModuleNames.AddRange(
		[
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore"
		]);

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
			[
				"UnrealEd"
			]);
		}
	}
}
