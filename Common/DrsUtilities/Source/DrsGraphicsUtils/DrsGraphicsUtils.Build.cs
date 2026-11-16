// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsGraphicsUtils : ModuleRules
{
	public DrsGraphicsUtils(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		// The view fade draws with a scene view extension (RenderCore, RHI) and converts Blueprint delegates (DrsBase).
		PrivateDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI", "DrsBase" });

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
	}
}
