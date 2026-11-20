// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiter.h"

/** The runtime probe. Only the arbiter module calls it; matching its result is Drs::XrArbiter::MatchVendorFromProbe. */
namespace Drs::XrArbiter
{
	/**
	 * Creates a bare OpenXR instance (no extensions except XR_KHR_android_create_instance on Android), reads the runtime
	 * name, and the system name and vendor ID when a headset is connected, then destroys it. Runs once; later calls return
	 * the first result. Unset when the loader is missing or the runtime refuses the instance (SteamVR with no headset);
	 * SystemName is empty when the runtime has no system (Oculus with no headset).
	 * Call it before the engine creates its own instance, at PostConfigInit. On Windows, it starts SteamVR when SteamVR is
	 * the selected runtime and isn't running yet.
	 */
	const TOptional<FXrRuntimeProbe>& ProbeXrRuntime();
}
