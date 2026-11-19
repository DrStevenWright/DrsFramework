// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

DRSXRARBITER_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrArbiter, Log, All);

/**
 * The XR arbiter: it selects the desktop OpenXR runtime and the vendor XR plugin, independently, and holds the early XR
 * state that DrsXrPlatform reads. Loads at EarliestPossible (Win64 and Android).
 *
 * THE OPENXR RUNTIME (Windows only)
 * At startup, before OpenXR creates its instance, it picks a runtime from the runtime table
 * ([/Script/DrsXrArbiter.DrsXrRuntimeSettings]) and points XR_RUNTIME_JSON at it. A running runtime wins; otherwise the
 * first installed one in RuntimePriority. Under -noHMD none is selected. On Android the device provides the runtime.
 *
 * THE VENDOR PLUGIN
 * Vendor XR plugins (Meta, Pico, ...) each hook into OpenXR at startup and can conflict when several are enabled. The
 * arbiter lets them all be enabled in one project and starts only the modules of the vendor that fits the device. It is
 * optional: a vendor plugin that is not integrated simply loads as normal, and with nothing integrated the arbiter starts
 * no vendor modules itself. Today no vendor plugin is integrated: the MetaXR fork loads as normal. GetXrPluginType() reports the running
 * vendor either way.
 *
 * INTEGRATING A VENDOR PLUGIN
 * 1. In the vendor's .uplugin, for each module to arbitrate:
 *      "LoadingPhase": "None",                     // The engine no longer loads it...
 *      "ArbitratedLoadingPhase": "PostConfigInit", // ...the arbiter loads it in its original phase instead.
 *    and name the vendor once at the top level ("ArbitratedXrPlugin": "Meta"), or per module to override it. The name
 *    must be a vendor's Id in the vendor table (step 3). EarliestPossible modules cannot be arbitrated (the arbiter may
 *    not be ready), so leave those alone. Text after the phase name is ignored, so "PostConfigInit (note)" is allowed.
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
 * 3. Add the vendor to the vendor table, [/Script/DrsXrArbiter.DrsXrArbiterSettings] Vendors in DefaultDrsXrPlatform.ini
 *    (the plugin's, or a project's to add it for that project only): its Id, the keywords that identify its hardware
 *    (matched against the device name, or on Android the brand and manufacturer), and its marker module. If it has a
 *    desktop runtime of its own, add that to [/Script/DrsXrArbiter.DrsXrRuntimeSettings] with Vendor set to the Id, and
 *    list it in RuntimePriority.
 * 4. Mark each change in the vendor plugin as a local patch (the fork uses "DRS:" comments, or a "DrsNote" key in
 *    JSON), so that it survives vendor upgrades.
 *
 * WHAT IT DOES NOT DO
 * It only decides what starts at runtime. Every enabled plugin is still compiled and packaged: Android UPL files are
 * applied, and libraries staged, whatever the arbiter later selects. To leave a vendor out of a build, disable the plugin.
 *
 * Choosing the vendor: see DrsXrPlatform/CLAUDE.md. Overrides: the editor setting (UDrsXrPlatformEditorSettings), the
 * -XrPlugin=<Id> command line, and XrArbiterProjectOverride. Commandlets load every vendor, so everything cooks.
 */
namespace Drs::XrArbiter
{
	/** The runtime name recorded on Android, where the device provides the runtime. */
	DRSXRARBITER_API extern const FName AndroidRuntime;

	/**
	 * The vendor XR plugin that is running, detected from its marker module in the vendor table. This works whether the
	 * arbiter loaded the plugin or the engine did, and is None without a vendor plugin. Vendor modules load at
	 * PostConfigInit, so before that it is always None.
	 */
	DRSXRARBITER_API FName GetXrPluginType();

	/** The vendor the arbiter selected, or None if it has not decided (or nothing is integrated with it). */
	DRSXRARBITER_API FName GetArbitratedXrPluginType();

	/** Whether any modules of a vendor are integrated with the arbiter (rather than loading as normal). */
	DRSXRARBITER_API bool IsXrPluginTypeArbitrated(FName vendorId);

	/** The vendor forced by the editor settings, the command line (-XrPlugin=) or the project settings, if any. */
	DRSXRARBITER_API FName GetXrPluginOverride(EDrsXrPluginOverrideType& outOverrideType);

	/**
	 * The vendor the connected hardware needs, ignoring any override (None if the hardware is not recognised). Tethered
	 * devices are identified by deviceName; Android devices by their brand and manufacturer.
	 */
	DRSXRARBITER_API FName GetExpectedXrPluginType(const FString& deviceName);

	/** The vendor table from config, read once. */
	DRSXRARBITER_API const TArray<FDrsXrVendor>& GetVendors();
	/** A vendor in the vendor table, or null. */
	DRSXRARBITER_API const FDrsXrVendor* FindVendor(FName vendorId);

	/** Parses the vendor table from its config entries, skipping invalid ones. */
	DRSXRARBITER_API TArray<FDrsXrVendor> ParseVendors(const TArray<FString>& entries);
	/** The first vendor with a keyword found in any of the identities (not case-sensitive), or None. */
	DRSXRARBITER_API FName MatchVendor(const TArray<FDrsXrVendor>& vendors, const TArray<FString>& identities);

	/** Parses the runtime table from its config entries, skipping invalid ones. */
	DRSXRARBITER_API TArray<FDrsXrRuntime> ParseRuntimes(const TArray<FString>& entries);
	/** The runtimes named in the priority, in its order. Names with no runtime are logged and skipped. */
	DRSXRARBITER_API TArray<FDrsXrRuntime> OrderRuntimes(const TArray<FDrsXrRuntime>& runtimes, const TArray<FName>& priority);
	/** The runtimes from config (UDrsXrRuntimeSettings), in priority order. */
	DRSXRARBITER_API TArray<FDrsXrRuntime> GetPrioritisedRuntimes();

	/**
	 * The runtime to select from runtimes in priority order: the first running one that is installed, otherwise the
	 * first installed one. findManifest returns a runtime's manifest path, or an empty string when it is not installed.
	 * Returns null when none is installed.
	 */
	DRSXRARBITER_API const FDrsXrRuntime* ChooseRuntime(const TArray<FDrsXrRuntime>& runtimes,
		TFunctionRef<FString(const FDrsXrRuntime&)> findManifest, TFunctionRef<bool(const FDrsXrRuntime&)> isRunning, FString& outManifest);

	/** The selected OpenXR runtime (AndroidRuntime on Android; None on desktop when none is selected). */
	DRSXRARBITER_API FName GetXrRuntimeType();
	/** The vendor the selected runtime implies, or None for a multi-vendor runtime such as SteamVR. */
	DRSXRARBITER_API FName GetXrRuntimeVendor();

	/**
	 * Whether the app runs without VR: -noHMD on the command line, the editor without VR Preview, or no HMD connected.
	 * Assumes VR until it knows otherwise.
	 */
	DRSXRARBITER_API bool IsNonVrMode();
}
