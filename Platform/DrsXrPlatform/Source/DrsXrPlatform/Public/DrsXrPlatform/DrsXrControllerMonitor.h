// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrPlatform/DrsXrControllerType.h"
#include "InputCoreTypes.h"

namespace Drs::XrPlatform
{
	/** The live hand-tracking queries FXrControllerMonitor needs. An interface, so tests can run without a headset. */
	class IXrControllerSource
	{
	public:
		virtual ~IXrControllerSource() = default;

		/** Whether the device and runtime can track hands at all. */
		virtual bool CanTrackHands() const = 0;

		/** Whether a tracked hand's joints are tracked this frame. */
		virtual bool IsTrackedHandJointTracked(EControllerHand hand) const = 0;

		/** Whether the hand's physical controller grip pose is tracked this frame. */
		virtual bool IsPhysicalControllerGripTracked(EControllerHand hand) const = 0;

		/** Whether mixed controller modes are enabled for this session, so the hands may be in different modes. */
		virtual bool AreMixedControllerModesEnabled() const = 0;
	};

	/**
	 * Decides each hand's controller mode from its interaction profile. When a hand has no profile (the runtime lacks
	 * XR_EXT_hand_interaction, or the controller is not powered on yet), joint tracking decides instead.
	 *
	 * Unless mixed controller modes are enabled, both hands share one mode: any physical controller profile means physical
	 * controllers for both hands. Local-only: holds no replicated state.
	 */
	class DRSXRPLATFORM_API FXrControllerMonitor
	{
	public:
		explicit FXrControllerMonitor(const IXrControllerSource& source) : Source(source) {}

		/** Call whenever the interaction profiles change (XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED). */
		void SetProfiles(const FString& leftProfile, const FString& rightProfile);

		/** Call every frame. Returns whether a hand's mode or physical controller profile changed since the last call. */
		bool Update(float deltaTime);

		/** Call when the OpenXR session is destroyed. */
		void Reset();

		const FDrsXrControllerState& GetState(EControllerHand hand) const { return (hand == EControllerHand::Left) ? LeftState : RightState; }

		/** Whether the hand has a controller: an active physical controller or a tracked hand that has been seen. */
		bool IsControllerPresent(EControllerHand hand) const;

		/** Whether either hand is in tracked-hand mode. */
		bool IsAnyHandTracked() const { return LeftState.Mode == EDrsXrControllerMode::TrackedHand || RightState.Mode == EDrsXrControllerMode::TrackedHand; }

		/**
		 * Whether the hand is driven by a physical controller: it has a physical controller profile and its grip has been
		 * tracked since the profile appeared. It turns inactive once the profile has been gone for PhysicalControllerInactiveDelay.
		 * This doesn't say whether the controller is powered: one put down for hand tracking turns inactive while still on.
		 */
		bool IsPhysicalControllerActive(EControllerHand hand) const { return GetPresence(hand).bPhysicalControllerActive; }

		/** How long a hand may lack a physical controller profile before its controller counts as inactive (seconds). Covers
		 *  the empty profile Meta reports for up to about 480 ms while switching between controller and tracked hand. */
		static constexpr float PhysicalControllerInactiveDelay = 0.75f;

	private:
		struct FControllerPresence
		{
			bool bTrackedHandSeen = false; // Since the hand entered tracked-hand mode.
			bool bPhysicalControllerActive = false;
			float TimeWithoutPhysicalControllerProfile = 0.0f; // Since the controller was active.
		};

		void SetProfile(FDrsXrControllerState& state, const TCHAR* handName, const FString& profile);
		void UpdatePresence(const FDrsXrControllerState& state, FControllerPresence& presence, EControllerHand hand, const TCHAR* handName, float deltaTime);
		const FControllerPresence& GetPresence(EControllerHand hand) const { return (hand == EControllerHand::Left) ? LeftPresence : RightPresence; }
		TOptional<EDrsXrControllerMode> GetCandidateMode(const FDrsXrControllerState& state, EControllerHand hand) const;

		const IXrControllerSource& Source;
		FDrsXrControllerState LeftState;
		FDrsXrControllerState RightState;
		FControllerPresence LeftPresence;
		FControllerPresence RightPresence;
		bool bPhysicalControllerProfileChanged = false;
	};
}
