// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrPlatform/DrsXrControllerMonitor.h"
#include "IOpenXRExtensionPlugin.h"
#include <openxr/openxr.h>
#include "Tickable.h"

class IHandTracker;
class IMotionController;

namespace Drs::XrPlatform
{
	/**
	 * The state of the hands and controllers, tracked through OpenXR: each hand's controller mode (FXrControllerMonitor),
	 * hand-tracking availability, and the aim pose of tracked hands. It also uses the OpenXR extensions for tracked hands
	 * that the engine doesn't, each only where the runtime enables it, whoever provides it:
	 * - XR_EXT_hand_interaction: bindings for its interaction profile, so that the runtime reports the profile while
	 *   hands are tracked (it only reports profiles the app suggested bindings for), and the monitor sees tracked-hand
	 *   mode.
	 * - XR_FB_hand_tracking_aim: the runtime's stabilised aim pose for tracked hands, read through hand trackers of its
	 *   own, since the engine's can't carry the aim state.
	 * - XR_META_simultaneous_hands_and_controllers: one hand on a controller while the other is tracked.
	 * Owned by FXrDeviceExtensions, and registered as an OpenXR extension plugin of its own. Local-only: holds no
	 * replicated state.
	 */
	class FXrControllerExtensions : public IOpenXRExtensionPlugin, public FTickableGameObject, public IXrControllerSource
	{
	public:
		FXrControllerExtensions();
		virtual ~FXrControllerExtensions() override;

		const FDrsXrControllerType& GetControllerType();
		bool IsControllerPresent(EDrsHandMask hands) const { return ControllerMonitor.IsControllerPresent(hands); }
		bool IsPhysicalControllerActive(EDrsHandMask hands) const { return ControllerMonitor.IsPhysicalControllerActive(hands); }
		bool IsHandTrackingAvailable() const;
		bool IsHandTrackingActive(EDrsHandMask hands) const { return ControllerMonitor.IsHandTracked(hands); }
		bool HasTrackedHandAimPose() const;
		bool GetTrackedHandAimPose(EDrsHand hand, FTransform& outWorldTransform) const;

		// FTickableGameObject
		virtual void Tick(float deltaTime) override;
		virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FXrControllerExtensions, STATGROUP_Tickables); }
		virtual bool IsAllowedToTick() const override { return !UE_SERVER; }

		// IOpenXRExtensionPlugin
		virtual FString GetDisplayName() override { return TEXT("DrsXrControllerExtensions"); }
		virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& outExtensions) override;
		virtual void PostCreateInstance(XrInstance inInstance) override;
		virtual const void* OnCreateSession(XrInstance inInstance, XrSystemId inSystem, const void* inNext) override;
		virtual void PostCreateSession(XrSession inSession) override;
		virtual void OnDestroySession(XrSession inSession) override;
		virtual void OnEvent(XrSession inSession, const XrEventDataBaseHeader* inHeader) override;
		virtual void UpdateDeviceLocations(XrSession inSession, XrTime displayTime, XrSpace trackingSpace) override;
		virtual void AttachActionSets(TSet<XrActionSet>& outActionSets) override;
		virtual void GetActiveActionSetsForSync(TArray<XrActiveActionSet>& outActiveSets) override;

		// IXrControllerSource
		virtual bool CanTrackHands() const override { return IsHandTrackingAvailable(); }
		virtual bool AreMixedControllerModesEnabled() const override { return MixedControllerModesEnabled.Get(false); }
		virtual bool IsTrackedHandJointTracked(EDrsHand hand) const override;
		virtual bool IsPhysicalControllerGripTracked(EDrsHand hand) const override;

	private:
		/** Updates the controller type, and broadcasts FXrDelegates::OnControllerTypeChanged when it changes. */
		void SetControllerStates(const FDrsXrControllerState& left, const FDrsXrControllerState& right);

		TOptional<bool> DetermineHandTrackingAvailability() const;
		TOptional<bool> IsHandTrackingSupportedForDevice() const; // Unset until the OpenXR session exists.
		void SuggestHandInteractionBindings(XrInstance instance);
		void EnableMixedControllerModes();
		void DestroyHandTrackers();
		const IHandTracker* GetHandTracker() const;
		const IMotionController* GetMotionController() const;

		XrSession Session = XR_NULL_HANDLE;
		FDrsXrControllerType ControllerType;
		FXrControllerMonitor ControllerMonitor{ *this };
		mutable TOptional<bool> HandTrackingAvailable; // Unset until known for the current session.
		bool bIsHandTrackingSupported = false; // From the system properties, for the aim hand trackers.

		// Cached for the per-frame joint queries.
		mutable const IHandTracker* CachedHandTracker = nullptr;
		mutable const IMotionController* CachedMotionController = nullptr;
		mutable TArray<FVector> JointPositions;
		mutable TArray<FQuat> JointRotations;
		mutable TArray<float> JointRadii;

		// Hand interaction profile (XR_EXT_hand_interaction). Null when no bindings were suggested. Destroyed with the instance.
		XrActionSet HandInteractionActionSet = XR_NULL_HANDLE;

		// Hand aim (XR_FB_hand_tracking_aim). The trackers exist only while it is usable. Index 0 is left, 1 is right.
		bool bIsAimEnabled = false;
		XrHandTrackerEXT HandTrackers[2] = { XR_NULL_HANDLE, XR_NULL_HANDLE };
		bool bIsAimValid[2] = { false, false };
		XrPosef AimPoses[2] = {};
		XrHandJointLocationEXT JointLocations[XR_HAND_JOINT_COUNT_EXT] = {}; // Scratch space; only the aim state is kept.
		PFN_xrCreateHandTrackerEXT CreateHandTracker = nullptr;
		PFN_xrDestroyHandTrackerEXT DestroyHandTracker = nullptr;
		PFN_xrLocateHandJointsEXT LocateHandJoints = nullptr;

		// Simultaneous hands and controllers (XR_META_simultaneous_hands_and_controllers). The resume function is in the .cpp.
		bool bIsSimultaneousHandsAndControllersEnabled = false;
		bool bSimultaneousHandsAndControllersSupported = false;
		TOptional<bool> MixedControllerModesEnabled; // Unset until decided for the session (also after a failed attempt, which is retried).
	};
}
