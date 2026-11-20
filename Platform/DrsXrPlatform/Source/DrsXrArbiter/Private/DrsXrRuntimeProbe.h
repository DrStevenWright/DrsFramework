// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiter.h"

/** Called by the arbiter system to probe the OpenXR runtime before any OpenXR instance has been created. */
namespace Drs::XrArbiter
{
	/**
	 * Creates a bare OpenXR instance (no extensions except XR_KHR_android_create_instance on Android), reads the runtime
	 * name, and the system name and vendor ID when a headset is connected, then destroys it. Runs once; later calls return
	 * the first result. Result is not set if the loader is missing or the runtime refuses the instance (e.g. SteamVR with
	 * no headset); or if SystemName is empty. Call this before the engine creates its own instance, at PostConfigInit.
	 * On Windows, this will start SteamVR if no runtime is selected and SteamVR is already the system's active runtime
	 * (a selected runtime is already running).
	 */
	const TOptional<FXrRuntimeProbe>& ProbeXrRuntime();
}
