// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs::XrRuntime
{
	/** Name of the environment variable the OpenXR loader reads to override the active runtime. */
	inline const TCHAR* RuntimeJsonVariable = TEXT("XR_RUNTIME_JSON");

	/**
	 * Finds the Meta (Oculus) OpenXR runtime manifest. Looks under the OculusBase install folder first, then in the
	 * runtimes registered with the OpenXR loader. Returns an empty string when it is not installed.
	 */
	FString FindOculusRuntimeJson();

	/** Sets XR_RUNTIME_JSON for this process. */
	void SetRuntimeJson(const FString& RuntimeJson);
}
