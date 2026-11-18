// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsBase/DrsHand.h"
#include "InputCoreTypes.h"

namespace Drs::Input
{
	/**
	 * Definition of a device-agnostic motion controller input key. A global input mapping context binds each platform's
	 * physical controller inputs to input actions whose ID matches ActionID. A UDrsMotionControllerInputComponent then
	 * forwards those actions as InputKey, and all gameplay input mapping contexts map to these keys instead of to
	 * platform-specific ones.
	 */
	struct DRSINPUT_API FMotionInputKey
	{
		FMotionInputKey() = default;
		FMotionInputKey(const FName keyName, const FString& displayName)
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
	struct DRSINPUT_API FMotionControllerHandKeys
	{
		explicit FMotionControllerHandKeys(EDrsHand hand);

		FMotionInputKey Menu;
		FMotionInputKey Grip;
		FMotionInputKey GripAxis;
		FMotionInputKey Trigger;
		FMotionInputKey TriggerTouch;
		FMotionInputKey TriggerAxis;
		FMotionInputKey Button1;
		FMotionInputKey Button1Touch;
		FMotionInputKey Button2;
		FMotionInputKey Button2Touch;
		FMotionInputKey Thumb;
		FMotionInputKey ThumbTouch;
		FMotionInputKey ThumbstickX;
		FMotionInputKey ThumbstickY;
		FMotionInputKey Thumbstick2D; // Paired with ThumbstickX and ThumbstickY by the engine, so it needs no input of its own.
	};

	/** Gets the keys for a hand. */
	DRSINPUT_API const FMotionControllerHandKeys& GetMotionControllerKeysForHand(EDrsHand hand);

	/** Registers all of the keys with the engine's key table. Called by the DrsInput module at startup. */
	void RegisterMotionControllerKeys();

	/** Removes the keys again. Called by the DrsInput module at shutdown. */
	void UnregisterMotionControllerKeys();
}
