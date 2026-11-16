// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrControllerMonitor.h"
#include "DrsLog.h"
#include "DrsXrPlatform.h"

namespace
{
	FString ToString(EDrsXrControllerMode mode)
	{
		return StaticEnum<EDrsXrControllerMode>()->GetNameStringByValue(static_cast<int64>(mode));
	}
}

void FDrsXrControllerMonitor::SetProfiles(const FString& leftProfile, const FString& rightProfile)
{
	SetProfile(LeftState, TEXT("Left"), leftProfile);
	SetProfile(RightState, TEXT("Right"), rightProfile);
}

void FDrsXrControllerMonitor::SetProfile(FDrsXrControllerState& state, const TCHAR* handName, const FString& profile)
{
	if (state.ProfilePath == profile)
	{
		return;
	}

	state.ProfileMode = Drs::XrInteractionProfiles::DetermineControllerMode(profile);
	DRS_LOG(LogDrsXrPlatform, Log, "%s interaction profile: '%s' -> '%s' (%s).", handName, *state.ProfilePath, *profile, *ToString(state.ProfileMode));
	state.ProfilePath = profile;

	if (state.ProfileMode == EDrsXrControllerMode::PhysicalController && profile != state.PhysicalControllerProfilePath)
	{
		state.PhysicalControllerProfilePath = profile;
		bPhysicalControllerProfileChanged = true;
	}
}

bool FDrsXrControllerMonitor::Update(float deltaTime)
{
	const TOptional<EDrsXrControllerMode> leftCandidate = GetCandidateMode(LeftState, EControllerHand::Left);
	const TOptional<EDrsXrControllerMode> rightCandidate = GetCandidateMode(RightState, EControllerHand::Right);

	EDrsXrControllerMode leftMode;
	EDrsXrControllerMode rightMode;
	if (Source.AreMixedControllerModesEnabled())
	{
		// Each hand takes its own candidate mode, or keeps its current one.
		leftMode = leftCandidate.Get(LeftState.Mode);
		rightMode = rightCandidate.Get(RightState.Mode);
	}
	else
	{
		// One mode for both hands. A physical controller on either hand wins.
		auto isEitherHand = [&leftCandidate, &rightCandidate](EDrsXrControllerMode candidate)
		{
			return (leftCandidate.IsSet() && leftCandidate.GetValue() == candidate) || (rightCandidate.IsSet() && rightCandidate.GetValue() == candidate);
		};

		EDrsXrControllerMode mode = LeftState.Mode;
		if (isEitherHand(EDrsXrControllerMode::PhysicalController))
		{
			mode = EDrsXrControllerMode::PhysicalController;
		}
		else if (isEitherHand(EDrsXrControllerMode::TrackedHand))
		{
			mode = EDrsXrControllerMode::TrackedHand;
		}
		leftMode = mode;
		rightMode = mode;
	}

	auto setMode = [](FDrsXrControllerState& state, EDrsXrControllerMode mode, const TCHAR* handName)
	{
		if (mode == state.Mode)
		{
			return false;
		}
		DRS_LOG(LogDrsXrPlatform, Log, "%s controller mode: %s (interaction profile: '%s').", handName, *ToString(mode), *state.ProfilePath);
		state.Mode = mode;
		return true;
	};
	const bool hasLeftModeChanged = setMode(LeftState, leftMode, TEXT("Left"));
	const bool hasRightModeChanged = setMode(RightState, rightMode, TEXT("Right"));

	UpdatePresence(LeftState, LeftPresence, EControllerHand::Left, TEXT("Left"), deltaTime);
	UpdatePresence(RightState, RightPresence, EControllerHand::Right, TEXT("Right"), deltaTime);

	const bool hasChanged = hasLeftModeChanged || hasRightModeChanged || bPhysicalControllerProfileChanged;
	bPhysicalControllerProfileChanged = false;
	return hasChanged;
}

void FDrsXrControllerMonitor::UpdatePresence(const FDrsXrControllerState& state, FControllerPresence& presence, EControllerHand hand, const TCHAR* handName, float deltaTime)
{
	// Physical controllers turn active once their grip is tracked, and inactive after a while without their profile.
	if (state.ProfileMode == EDrsXrControllerMode::PhysicalController)
	{
		presence.TimeWithoutPhysicalControllerProfile = 0.0f;
		if (!presence.bPhysicalControllerActive && Source.IsPhysicalControllerGripTracked(hand))
		{
			DRS_LOG(LogDrsXrPlatform, Log, "%s physical controller active.", handName);
			presence.bPhysicalControllerActive = true;
		}
	}
	else if (presence.bPhysicalControllerActive)
	{
		presence.TimeWithoutPhysicalControllerProfile += deltaTime;
		if (presence.TimeWithoutPhysicalControllerProfile >= PhysicalControllerInactiveDelay)
		{
			DRS_LOG(LogDrsXrPlatform, Log, "%s physical controller inactive (controller profile: '%s', current profile: '%s').", handName, *state.PhysicalControllerProfilePath, *state.ProfilePath);
			presence.bPhysicalControllerActive = false;
			presence.TimeWithoutPhysicalControllerProfile = 0.0f;
		}
	}

	// A tracked hand counts as present once its joints have been tracked in tracked-hand mode.
	if (state.Mode != EDrsXrControllerMode::TrackedHand)
	{
		presence.bTrackedHandSeen = false;
	}
	else if (!presence.bTrackedHandSeen && Source.IsTrackedHandJointTracked(hand))
	{
		DRS_LOG(LogDrsXrPlatform, Log, "%s tracked hand seen.", handName);
		presence.bTrackedHandSeen = true;
	}
}

TOptional<EDrsXrControllerMode> FDrsXrControllerMonitor::GetCandidateMode(const FDrsXrControllerState& state, EControllerHand hand) const
{
	if (state.ProfileMode != EDrsXrControllerMode::Unknown)
	{
		return state.ProfileMode;
	}

	// No profile, so the runtime isn't saying. Tracked joints mean tracked-hand mode.
	if (Source.CanTrackHands() && Source.IsTrackedHandJointTracked(hand))
	{
		return EDrsXrControllerMode::TrackedHand;
	}

	return {}; // Keep the current mode: joint dropouts never change it.
}

void FDrsXrControllerMonitor::Reset()
{
	LeftState = FDrsXrControllerState();
	RightState = FDrsXrControllerState();
	LeftPresence = FControllerPresence();
	RightPresence = FControllerPresence();
	bPhysicalControllerProfileChanged = false;
}

bool FDrsXrControllerMonitor::IsControllerPresent(EControllerHand hand) const
{
	const FControllerPresence& presence = GetPresence(hand);
	return presence.bPhysicalControllerActive || presence.bTrackedHandSeen;
}
