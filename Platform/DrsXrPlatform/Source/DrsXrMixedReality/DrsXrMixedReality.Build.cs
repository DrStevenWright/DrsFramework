// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsXrMixedReality : ModuleRules
{
	public DrsXrMixedReality(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(["Core", "CoreUObject", "Engine", "DrsBase"]);

		PrivateDependencyModuleNames.AddRange(["DrsGraphicsUtils"]);

		// Passthrough is only supported on standalone (Android) headsets.
		var IsPassthroughSupported = Target.Platform == UnrealTargetPlatform.Android && Target.Type != TargetType.Server;
		PrivateDefinitions.Add("DRS_PASSTHROUGH_SUPPORTED=" + (IsPassthroughSupported ? "1" : "0"));

		if (IsPassthroughSupported)
		{
			PrivateDependencyModuleNames.AddRange(["HeadMountedDisplay", "OpenXRHMD", "RenderCore", "RHI", "DrsXrPlatform"]);
			AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");
		}
	}
}
