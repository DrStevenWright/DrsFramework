// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSXRRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrRuntime, Log, All);

/**
 * Selects the desktop OpenXR runtime before OpenXR creates its instance, by setting XR_RUNTIME_JSON for this process.
 * Loads at EarliestPossible on Win64 (OpenXRHMD creates its instance at PostConfigInit).
 * Currently always selects the Meta (Oculus) runtime when it is installed, overriding the system's active runtime.
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
