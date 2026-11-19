// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrRuntime, Log, All);

namespace Drs::XrArbiter
{
	/** Records the selected runtime and the vendor it implies. */
	void SetXrRuntime(FName runtimeName, FName vendorId);
}

/** Desktop OpenXR runtime selection. Only the arbiter module calls it; the selection rule itself is Drs::XrArbiter::ChooseRuntime. */
namespace Drs::XrRuntime
{
	/** Name of the environment variable the OpenXR loader reads to override the active runtime. */
	inline const TCHAR* RuntimeJsonVariable = TEXT("XR_RUNTIME_JSON");

	/**
	 * Selects the desktop OpenXR runtime before OpenXR creates its instance (at PostConfigInit), by setting
	 * XR_RUNTIME_JSON for this process, and records it with Drs::XrArbiter::SetXrRuntime. The runtimes and their priority
	 * come from UDrsXrRuntimeSettings. A running runtime wins; otherwise the first installed one in order of priority.
	 * Under -noHMD no runtime is selected. Does nothing except on Windows clients and the editor.
	 */
	void SelectRuntime();

	/** The runtime's manifest path from the first of its locators that finds one, or an empty string when it is not installed. */
	FString FindRuntimeJson(const FDrsXrRuntime& runtime);

	/** Whether any of the runtime's RunningProcesses is running. */
	bool IsRuntimeRunning(const FDrsXrRuntime& runtime);
}
