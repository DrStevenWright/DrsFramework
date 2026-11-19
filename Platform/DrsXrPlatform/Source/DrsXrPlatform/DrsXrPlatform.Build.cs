// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

using System.IO;
using UnrealBuildTool;

public class DrsXrPlatform : ModuleRules
{
	public DrsXrPlatform(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "DeveloperSettings", "Engine", "HeadMountedDisplay", "InputCore", "DrsBase", "DrsXrArbiter" });

		PrivateDependencyModuleNames.AddRange(new string[] { "OpenXRHMD", "OpenXRHandTracking", "Projects", "XRBase" });

		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}

		// DRS's Android manifest entries, for any vendor (today the Meta Quest hand-tracking and FrameSync entries).
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "DrsXrPlatform_UPL.xml"));
		}
	}
}
