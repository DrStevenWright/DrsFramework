// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "DrsXrPlatformSubsystem.generated.h"

/**
 * Engine-lifetime entry point to the XR platform layer.
 * Local-only: holds no replicated state.
 */
UCLASS()
class DRSXRPLATFORM_API UDrsXrPlatformSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** True when the engine has an active XR tracking system (e.g. OpenXR). */
	UFUNCTION(BlueprintPure, Category = "Drs|XR Platform")
	bool IsXrSystemActive() const;

	/** Name of the active XR tracking system, or None. */
	UFUNCTION(BlueprintPure, Category = "Drs|XR Platform")
	FName GetXrSystemName() const;

	/**
	 * Puts the tracking-space origin on the floor, centred where the user started (OpenXR LOCAL_FLOOR;
	 * the runtime falls back to LOCAL if unsupported). Returns false when no XR system is active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Drs|XR Platform")
	bool SetFloorTrackingOrigin();
};
