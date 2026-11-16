// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsXrHandTrackingMeta : ModuleRules
{
	public DrsXrHandTrackingMeta(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "HeadMountedDisplay", "InputCore", "OpenXRHMD", "DrsXrPlatform", "OculusXRHMD", "OculusXRInput" });

		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");
	}
}
