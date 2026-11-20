// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsXrArbiter : ModuleRules
{
	public DrsXrArbiter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "DeveloperSettings" });

		PrivateDependencyModuleNames.AddRange(new string[] { "DrsBase", "Engine", "HeadMountedDisplay", "Json", "Projects", "XRBase" });

		// The OpenXR headers needed for the runtime probe.
		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
	}
}
