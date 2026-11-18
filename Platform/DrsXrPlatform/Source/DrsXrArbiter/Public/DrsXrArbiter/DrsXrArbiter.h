// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

DRSXRARBITER_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrArbiter, Log, All);

/**
 * The XR plugin arbiter, and the early XR state that DrsXrRuntime and DrsXrPlatform share.
 * Loads at EarliestPossible (Win64 and Android).
 *
 * WHAT IT IS FOR
 * Vendor XR plugins (Meta, Pico, ...) each hook into OpenXR at startup and can conflict when several are enabled. The
 * arbiter lets them all be enabled in one project and starts only the modules of the vendor that fits the device. It is
 * optional: a vendor plugin that is not integrated simply loads as normal, and with nothing integrated the arbiter does
 * nothing. Today no vendor plugin is integrated: the MetaXR fork loads as normal. GetXrPluginType() reports the running
 * vendor either way.
 *
 * INTEGRATING A VENDOR PLUGIN
 * 1. In the vendor's .uplugin, for each module to arbitrate:
 *      "LoadingPhase": "None",                     // The engine no longer loads it...
 *      "ArbitratedLoadingPhase": "PostConfigInit", // ...the arbiter loads it in its original phase instead.
 *    and name the vendor once at the top level ("ArbitratedXrPlugin": "Meta"), or per module to override it. The type
 *    must be an EDrsXrPluginType name. EarliestPossible modules cannot be arbitrated (the arbiter may not be ready), so
 *    leave those alone. Text after the phase name is ignored, so "PostConfigInit (note)" is allowed.
 * 2. Move code that must run whatever vendor is selected into a module that is NOT arbitrated (normal LoadingPhase,
 *    no ArbitratedLoadingPhase). The usual case is the plugin's shader directory: global shaders register as soon as
 *    their DLL is loaded, and the DLL loads whenever another module links it, even if the arbiter never starts it.
 *    Without the mapping the engine fails fatally when it checks shader sources. So map the directory from an exempt
 *    module, and make the vendor's own mapping skip if it already exists (adding it twice asserts):
 *      if (!AllShaderSourceDirectoryMappings().Contains(TEXT("/Plugin/Vendor")))
 *      {
 *          AddShaderSourceDirectoryMapping(TEXT("/Plugin/Vendor"), FPaths::Combine(PluginBaseDir, TEXT("Shaders")));
 *      }
 *    Look for anything else of this kind too: class or asset registration other code relies on, editor menus, cvars
 *    read by other plugins.
 * 3. Add arbitration rules for the vendor's devices to [/Script/DrsXrArbiter.DrsXrArbiterSettings] in Game.ini
 *    (MobileDeviceArbitrationRules for Android, TetheredDeviceArbitrationRules for Windows).
 * 4. Mark each change in the vendor plugin as a local patch (the fork uses "DRS:" comments, or a "DrsNote" key in
 *    JSON), so that it survives vendor upgrades.
 *
 * WHAT IT DOES NOT DO
 * It only decides what starts at runtime. Every enabled plugin is still compiled and packaged: Android UPL files are
 * applied, and libraries staged, whatever the arbiter later selects. To leave a vendor out of a build, disable the plugin.
 *
 * Choosing the type: see DrsXrPlatform/CLAUDE.md. Overrides: the editor setting (UDrsXrPlatformEditorSettings), the
 * -DrsXrPlugin=<Type> command line, and ProjectArbitrationOverride. Commandlets load every type, so everything cooks.
 */
namespace Drs::XrArbiter
{
	/**
	 * The vendor XR plugin that is running, detected from its loaded modules (OculusXRHMD for Meta, PICOOpenXRHMD for
	 * Pico). This works whether the arbiter loaded the plugin or the engine did, and is None without a vendor plugin.
	 * Vendor modules load at PostConfigInit, so before that it is always None.
	 */
	DRSXRARBITER_API EDrsXrPluginType GetXrPluginType();

	/** The plugin type the arbiter selected, or None if it has not decided (or nothing is integrated with it). */
	DRSXRARBITER_API EDrsXrPluginType GetArbitratedXrPluginType();

	/** Whether any modules of a plugin type are integrated with the arbiter (rather than loading as normal). */
	DRSXRARBITER_API bool IsXrPluginTypeArbitrated(EDrsXrPluginType pluginType);

	/** The plugin type forced by the editor settings, the command line (-DrsXrPlugin=) or the project settings, if any. */
	DRSXRARBITER_API EDrsXrPluginType GetXrPluginOverride(EDrsXrPluginOverrideType& outOverrideType);

	/**
	 * The plugin type the connected hardware needs according to the rules, ignoring any override (None if the hardware is
	 * not recognised). Tethered devices are identified by deviceName; Android devices by their device properties.
	 */
	DRSXRARBITER_API EDrsXrPluginType GetExpectedXrPluginType(const FString& deviceName);

	/** Whether a plugin type may be selected in this build (see DRS_ENABLE_META_XR, DRS_ENABLE_PICO_XR). */
	DRSXRARBITER_API bool IsXrPluginTypeEnabled(EDrsXrPluginType pluginType);

	/** Plugin type names as used in config, on the command line and in plugin descriptors. Safe before UObjects exist. */
	DRSXRARBITER_API FString PluginTypeToString(EDrsXrPluginType pluginType);
	/** Unset (with an error logged) when the name is not recognised. */
	DRSXRARBITER_API TOptional<EDrsXrPluginType> PluginTypeFromString(const FString& pluginTypeString);

	/** Records the runtime that DrsXrRuntime selected, and its EDrsXrPlatformProperty flags. */
	DRSXRARBITER_API void SetXrRuntime(EDrsXrRuntimeType runtimeType, uint32 runtimeFlags);
	/** The selected OpenXR runtime (Android on Android; None on desktop until DrsXrRuntime selects one). */
	DRSXRARBITER_API EDrsXrRuntimeType GetXrRuntimeType();
	/** The selected runtime's EDrsXrPlatformProperty flags (see ToRuntimeFlag). */
	DRSXRARBITER_API uint32 GetXrRuntimeFlags();

	/** Records the runtime the connected hardware is expected to use, where that differs from the one selected (e.g. a Pico headset over SteamVR). */
	DRSXRARBITER_API void SetExpectedXrRuntimeType(EDrsXrRuntimeType runtimeType);
	DRSXRARBITER_API EDrsXrRuntimeType GetExpectedXrRuntimeType();

	/** Runtime type names, for logging and arbitration rules. Safe before UObjects exist. */
	DRSXRARBITER_API FString RuntimeTypeToString(EDrsXrRuntimeType runtimeType);

	/**
	 * Whether the app runs without VR: -noHMD on the command line, the editor without VR Preview, or no HMD connected.
	 * Assumes VR until it knows otherwise.
	 */
	DRSXRARBITER_API bool IsNonVrMode();
}
