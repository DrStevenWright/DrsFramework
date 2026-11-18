// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatform/DrsXrConfiguration.h"

#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrPlatform/DrsXrPlatformLibrary.h"

namespace Drs::XrPlatform
{
	FXrConfiguration FXrConfiguration::Get()
	{
		FXrConfiguration configuration;
		configuration.XrDeviceName = UDrsXrPlatformLibrary::GetHmdDeviceName();
		configuration.XrRuntimeType = UDrsXrPlatformLibrary::GetXrRuntimeType();
		configuration.XrSelectedPlugin = Drs::XrArbiter::GetXrPluginType();
		configuration.XrExpectedPlugin = Drs::XrArbiter::GetExpectedXrPluginType(configuration.XrDeviceName.ToString());
		const EDrsXrPluginType overridePlugin = Drs::XrArbiter::GetXrPluginOverride(configuration.XrPluginOverrideType);

		if (configuration.XrSelectedPlugin == EDrsXrPluginType::None)
		{
			configuration.XrPluginSource = EXrPluginSource::None;
		}
		else if (configuration.XrPluginOverrideType != EDrsXrPluginOverrideType::None && overridePlugin == configuration.XrSelectedPlugin)
		{
			configuration.XrPluginSource = EXrPluginSource::Forced;
		}
		else if (Drs::XrArbiter::IsXrPluginTypeArbitrated(configuration.XrSelectedPlugin))
		{
			configuration.XrPluginSource = EXrPluginSource::Arbitrated;
		}
		else
		{
			configuration.XrPluginSource = EXrPluginSource::Loaded;
		}
		return configuration;
	}

	bool FXrConfiguration::IsValid() const
	{
		// Plain OpenXR, a plugin chosen from the hardware, and hardware the rules don't recognise all pass.
		if (XrPluginSource == EXrPluginSource::None || XrPluginSource == EXrPluginSource::Arbitrated || XrExpectedPlugin == EDrsXrPluginType::None)
		{
			return true;
		}

		if (XrSelectedPlugin != XrExpectedPlugin)
		{
			return false;
		}

		// Standalone devices have no choice of runtime.
		if (XrRuntimeType == EDrsXrRuntimeType::Android || XrRuntimeType == EDrsXrRuntimeType::None)
		{
			return true;
		}

		switch (XrExpectedPlugin)
		{
		case EDrsXrPluginType::Meta:
			// SteamVR is DrsXrRuntime's fallback when there is no Oculus runtime (e.g. Steam Link or Virtual Desktop), at the
			// cost of Meta-specific OpenXR extensions.
			return XrRuntimeType == EDrsXrRuntimeType::Oculus || XrRuntimeType == EDrsXrRuntimeType::SteamVR;

		case EDrsXrPluginType::Pico:
			return XrRuntimeType == EDrsXrRuntimeType::Pico || XrRuntimeType == EDrsXrRuntimeType::SteamVR;

		default:
			ensure(false);
			return true;
		}
	}
}
