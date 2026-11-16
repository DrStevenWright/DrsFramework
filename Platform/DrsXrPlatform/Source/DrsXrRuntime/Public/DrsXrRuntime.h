// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSXRRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrRuntime, Log, All);

/**
 * Selects the desktop OpenXR runtime before OpenXR creates its instance, by setting XR_RUNTIME_JSON for this process.
 * Loads at EarliestPossible on Win64 (OpenXRHMD creates its instance at PostConfigInit), and records the selection with
 * Drs::XrArbiter::SetXrRuntime. A running runtime wins; otherwise the first installed one in order of preference.
 * Pass -DrsKeepXrRuntime to leave the runtime selection alone.
 */
class DRSXRRUNTIME_API FDrsXrRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	/** The runtime manifest this module selected, or empty if it selected none. */
	const FString& GetSelectedRuntimeJson() const { return SelectedRuntimeJson; }

private:
	FString SelectedRuntimeJson;
};
