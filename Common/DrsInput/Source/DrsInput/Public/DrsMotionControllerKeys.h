// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsHand.h"
#include "InputCoreTypes.h"

/**
 * Definition of a device-agnostic motion controller input key. A global input mapping context binds each platform's
 * physical controller inputs to input actions whose ID matches ActionID. A UDrsMotionControllerInputComponent then
 * forwards those actions as InputKey, and all gameplay input mapping contexts map to these keys instead of to
 * platform-specific ones.
 */
struct DRSINPUT_API FDrsMotionInputKey
{
	FDrsMotionInputKey() = default;
	FDrsMotionInputKey(const FName keyName, const FString& displayName)
		: ActionID(keyName)
		, InputKey(keyName)
		, DisplayName(displayName)
	{
	}

	// Name used to find the input action (see UDrsInputAction::GetActionID) that supplies this key.
	FName ActionID;

	// The custom input key itself. (Its FName is the same as ActionID.)
	FKey InputKey;

	// How the input key is displayed in the editor.
	FString DisplayName;
};

/** The full set of device-agnostic input keys for one hand. */
struct DRSINPUT_API FDrsMotionControllerHandKeys
{
	explicit FDrsMotionControllerHandKeys(EDrsHand hand);

	FDrsMotionInputKey Menu;
	FDrsMotionInputKey Grip;
	FDrsMotionInputKey GripAxis;
	FDrsMotionInputKey Trigger;
	FDrsMotionInputKey TriggerTouch;
	FDrsMotionInputKey TriggerAxis;
	FDrsMotionInputKey Button1;
	FDrsMotionInputKey Button1Touch;
	FDrsMotionInputKey Button2;
	FDrsMotionInputKey Button2Touch;
	FDrsMotionInputKey Thumb;
	FDrsMotionInputKey ThumbTouch;
	FDrsMotionInputKey ThumbstickX;
	FDrsMotionInputKey ThumbstickY;
	FDrsMotionInputKey Thumbstick2D; // Paired with ThumbstickX and ThumbstickY by the engine, so it needs no input of its own.
};

namespace Drs::MotionControllerKeys
{
	/** Gets the keys for a hand. */
	DRSINPUT_API const FDrsMotionControllerHandKeys& Get(EDrsHand hand);

	/** Registers all of the keys with the engine's key table. Called by the DrsInput module at startup. */
	void Register();

	/** Removes the keys again. Called by the DrsInput module at shutdown. */
	void Unregister();
}
