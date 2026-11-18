// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

namespace Drs::XrRuntime
{
	/** Name of the environment variable the OpenXR loader reads to override the active runtime. */
	inline const TCHAR* RuntimeJsonVariable = TEXT("XR_RUNTIME_JSON");

	/** A desktop OpenXR runtime that can be selected at startup. */
	struct FRuntimeCandidate
	{
		EDrsXrRuntimeType Type = EDrsXrRuntimeType::None;
		/** EDrsXrPlatformProperty flags (see Drs::XrArbiter::ToRuntimeFlag). */
		uint32 Flags = 0;
		/** Returns the runtime's manifest path, or an empty string when it is not installed. */
		FString (*FindRuntimeJson)() = nullptr;
		/** Whether the runtime (or its client software) is running, in which case it takes precedence. */
		bool (*IsRunning)() = nullptr;
	};

	FRuntimeCandidate OculusRuntime();
	FRuntimeCandidate SteamVrRuntime();
	FRuntimeCandidate PicoStreamingRuntime();
	FRuntimeCandidate PicoConnectRuntime();

	/** Whether a Pico streaming client is running, which suggests that a Pico headset is connected. */
	bool IsPicoClientRunning();

	/** Sets XR_RUNTIME_JSON for this process. */
	void SetRuntimeJson(const FString& runtimeJson);
}
