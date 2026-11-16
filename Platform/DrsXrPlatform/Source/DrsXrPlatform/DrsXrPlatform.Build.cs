// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using UnrealBuildTool;

public class DrsXrPlatform : ModuleRules
{
	public DrsXrPlatform(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "HeadMountedDisplay", "InputCore", "DrsBase", "DrsXrArbiter" });

		PrivateDependencyModuleNames.AddRange(new string[] { "OpenXRHMD", "OpenXRHandTracking", "Projects", "XRBase" });

		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}

		// MetaXR (External), for Meta-only queries such as GPU utilisation. Building without it is a planned follow-up.
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "OculusXRHMD" });
		}
	}
}
