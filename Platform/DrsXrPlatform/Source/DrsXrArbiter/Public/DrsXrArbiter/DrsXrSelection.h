// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

namespace Drs::XrArbiter
{
	/** How the running vendor XR plugin came to be the one in use. */
	enum class EXrPluginSource : uint8
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

	/**
	 * Encapsulates the results of XR runtime selection and XR plugin arbitration for the purposes of validating against
	 * each other and against the connected hardware.
	 */
	struct DRSXRARBITER_API FXrSelection
	{
		/** The current selection. The device name is only known once the HMD is up. */
		static FXrSelection Get();

		/**
		 * False when the running vendor plugin doesn't fit the connected hardware or the selected runtime. A plugin the
		 * arbiter chose from the hardware always fits, and so does running without a vendor plugin (plain OpenXR). A forced
		 * or normally loaded plugin is checked against the hardware the vendor table expects, when it recognises it, and
		 * against the vendor the selected runtime implies.
		 */
		bool IsValid() const;

		/** Checks validity of the configuration, logging a warning when incompatibilities are detected. */
		bool Validate() const;

		/** The HMD's device name, or None before the HMD is up. */
		FName DeviceName;
		/** The selected runtime (GetXrRuntimeType). */
		FName Runtime;
		/** The vendor that the selected runtime implies, or None for a multi-vendor runtime such as SteamVR. */
		FName RuntimeVendor;
		/** The running vendor plugin (GetXrPluginType). */
		FName Plugin;
		/** How Plugin came to be running. */
		EXrPluginSource PluginSource = EXrPluginSource::None;
		/** The vendor that the connected hardware needs, by the vendor table (None if not recognised). */
		FName ExpectedPlugin;
		/** How the plugin type was forced, if it was. */
		EDrsXrPluginOverrideType PluginOverrideType = EDrsXrPluginOverrideType::None;
	};
}
