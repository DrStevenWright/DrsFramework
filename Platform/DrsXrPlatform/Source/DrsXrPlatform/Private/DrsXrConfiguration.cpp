// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrConfiguration.h"
#include "DrsXrArbiter.h"
#include "DrsXrPlatformLibrary.h"

FDrsXrConfiguration FDrsXrConfiguration::Get()
{
	FDrsXrConfiguration configuration;
	configuration.XrDeviceName = UDrsXrPlatformLibrary::GetHmdDeviceName();
	configuration.XrRuntimeType = UDrsXrPlatformLibrary::GetXrRuntimeType();
	configuration.XrSelectedPlugin = Drs::XrArbiter::GetXrPluginType();
	configuration.XrExpectedPlugin = Drs::XrArbiter::GetExpectedXrPluginType(configuration.XrDeviceName.ToString());
	const EDrsXrPluginType overridePlugin = Drs::XrArbiter::GetXrPluginOverride(configuration.XrPluginOverrideType);

	if (configuration.XrSelectedPlugin == EDrsXrPluginType::None)
	{
		configuration.XrPluginSource = EDrsXrPluginSource::None;
	}
	else if (configuration.XrPluginOverrideType != EDrsXrPluginOverrideType::None && overridePlugin == configuration.XrSelectedPlugin)
	{
		configuration.XrPluginSource = EDrsXrPluginSource::Forced;
	}
	else if (Drs::XrArbiter::IsXrPluginTypeArbitrated(configuration.XrSelectedPlugin))
	{
		configuration.XrPluginSource = EDrsXrPluginSource::Arbitrated;
	}
	else
	{
		configuration.XrPluginSource = EDrsXrPluginSource::Loaded;
	}
	return configuration;
}

bool FDrsXrConfiguration::IsValid() const
{
	// Plain OpenXR, a plugin chosen from the hardware, and hardware the rules don't recognise all pass.
	if (XrPluginSource == EDrsXrPluginSource::None || XrPluginSource == EDrsXrPluginSource::Arbitrated || XrExpectedPlugin == EDrsXrPluginType::None)
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
