// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsXrArbiterTypes.h"

/** How the running vendor XR plugin came to be the one in use. */
enum class EDrsXrPluginSource : uint8
{
	/** No vendor plugin is running. */
	None,
	/** The arbiter chose it from the hardware. */
	Arbitrated,
	/** An override (editor setting, command line or project setting) forced it. */
	Forced,
	/** It is not integrated with the arbiter and loaded as normal, so nothing matched it to the hardware. */
	Loaded,
};

/** The current XR runtime and plugin configuration, for validation. Local-only: holds no replicated state. */
struct DRSXRPLATFORM_API FDrsXrConfiguration
{
	static FDrsXrConfiguration Get();

	/**
	 * False when the running vendor plugin doesn't fit the connected hardware or the selected runtime. A plugin the arbiter
	 * chose from the hardware always fits, and so does running without a vendor plugin (plain OpenXR). A forced or normally
	 * loaded plugin is checked against the hardware the arbitration rules expect, when they recognise it.
	 */
	bool IsValid() const;

	FName XrDeviceName = NAME_None;
	EDrsXrRuntimeType XrRuntimeType = EDrsXrRuntimeType::None;
	/** The running vendor plugin (Drs::XrArbiter::GetXrPluginType). */
	EDrsXrPluginType XrSelectedPlugin = EDrsXrPluginType::None;
	/** How XrSelectedPlugin came to be in use. */
	EDrsXrPluginSource XrPluginSource = EDrsXrPluginSource::None;
	/** The plugin type the connected hardware needs, by the arbitration rules (None if not recognised). */
	EDrsXrPluginType XrExpectedPlugin = EDrsXrPluginType::None;
	/** How the plugin type was forced, if it was. */
	EDrsXrPluginOverrideType XrPluginOverrideType = EDrsXrPluginOverrideType::None;
};
