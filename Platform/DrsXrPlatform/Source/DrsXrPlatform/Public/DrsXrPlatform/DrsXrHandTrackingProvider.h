// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Features/IModularFeature.h"

enum class EControllerHand : uint8;

/**
 * Vendor-specific hand-tracking behaviour, registered as a modular feature by a module that the arbiter loads only for
 * its vendor (e.g. DrsXrHandTrackingMeta).
 */
class IDrsXrHandTrackingProvider : public IModularFeature
{
public:
	virtual ~IDrsXrHandTrackingProvider() = default;

	static FName GetModularFeatureName()
	{
		static const FName featureName(TEXT("DrsXrHandTrackingProvider"));
		return featureName;
	}

	/** For logging. */
	virtual FString GetName() const = 0;

	/** Whether the vendor's project settings enable hand tracking. */
	virtual bool IsHandTrackingEnabled() const = 0;

	/** Whether the runtime provides a usable aim pose for tracked hands. */
	virtual bool HasAimPose() const = 0;

	/** The world-space aim pose of a tracked hand. */
	virtual bool GetAimPose(EControllerHand hand, FTransform& outWorldTransform) const = 0;

	/** Whether mixed controller modes are supported: one hand on a physical controller while the other is tracked. */
	virtual bool AreMixedControllerModesSupported() const { return false; }

	/** Enables mixed controller modes for the running session. Returns whether it succeeded. */
	virtual bool EnableMixedControllerModes() { return false; }
};
