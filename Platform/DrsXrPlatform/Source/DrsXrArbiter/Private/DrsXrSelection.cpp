// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrSelection.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"
#include "HeadMountedDisplayFunctionLibrary.h"

namespace Drs::XrArbiter
{
	FXrSelection FXrSelection::Get()
	{
		FXrSelection selection;
		selection.DeviceName = UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName();
		selection.Runtime = GetXrRuntimeType();
		selection.RuntimeVendor = GetXrRuntimeVendor();
		selection.Plugin = GetXrPluginType();
		selection.ExpectedPlugin = GetExpectedXrPluginType(selection.DeviceName.ToString());
		const FName overridePlugin = GetXrPluginOverride(selection.PluginOverrideType);

		if (selection.Plugin.IsNone())
		{
			selection.PluginSource = EXrPluginSource::None;
		}
		else if (selection.PluginOverrideType != EDrsXrPluginOverrideType::None && overridePlugin == selection.Plugin)
		{
			selection.PluginSource = EXrPluginSource::Forced;
		}
		else if (IsXrPluginTypeArbitrated(selection.Plugin))
		{
			selection.PluginSource = EXrPluginSource::Arbitrated;
		}
		else
		{
			selection.PluginSource = EXrPluginSource::Loaded;
		}
		return selection;
	}

	bool FXrSelection::IsValid() const
	{
		// Plain OpenXR always passes.
		if (PluginSource == EXrPluginSource::None)
		{
			return true;
		}

		// The runtime is chosen without regard to the plugin, so check that it fits. Standalone devices have no choice of
		// runtime, and a multi-vendor runtime such as SteamVR fits any vendor (at the cost of vendor-specific extensions).
		const bool hasRuntimeChoice = Runtime != AndroidRuntime && !Runtime.IsNone();
		if (hasRuntimeChoice && !RuntimeVendor.IsNone() && RuntimeVendor != Plugin)
		{
			return false;
		}

		// A plugin chosen from the hardware fits it, and hardware the vendor table doesn't recognise can't be checked.
		return PluginSource == EXrPluginSource::Arbitrated || ExpectedPlugin.IsNone() || Plugin == ExpectedPlugin;
	}

	bool FXrSelection::Validate() const
	{
		const bool isConsistent = IsValid();
		DRS_CLOG(!isConsistent, LogDrsXrArbiter, Warning,
			"The XR plugin '%s' doesn't fit the hardware or runtime: device '%s' needs '%s', and the runtime '%s' implies '%s'.",
			*Plugin.ToString(), *DeviceName.ToString(), *ExpectedPlugin.ToString(), *Runtime.ToString(), *RuntimeVendor.ToString());
		return isConsistent;
	}
}
