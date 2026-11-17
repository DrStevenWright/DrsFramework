// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMotionControllerKeys.h"

namespace
{
	const FName MenuCategory("DrsMotionController");

	// NOTE: These key names are serialised into input mapping context assets (and matched against input action IDs), so
	// don't change them lightly.
	FDrsMotionInputKey MakeKey(EDrsHand hand, const char* keyName, const char* displayName)
	{
		const bool isLeft = (hand == EDrsHand::Left);
		const FString sideName = FString::Printf(TEXT("%hs_%hs"), isLeft ? "Left" : "Right", keyName);
		const FString display = FString::Printf(TEXT("Motion Controller (%s) %hs"), isLeft ? TEXT("Left") : TEXT("Right"), displayName);
		return FDrsMotionInputKey(FName(*sideName), display);
	}

	void AddKey(const FDrsMotionInputKey& key, const uint32 flags)
	{
		// EKeys::AddKey ensures on a key that is already registered, which would happen if the module were reloaded.
		if (!EKeys::GetKeyDetails(key.InputKey).IsValid())
		{
			EKeys::AddKey(FKeyDetails(key.InputKey, FText::FromString(key.DisplayName), flags, MenuCategory));
		}
	}

	void RegisterHand(const FDrsMotionControllerHandKeys& keys)
	{
		constexpr uint32 buttonFlags = FKeyDetails::GamepadKey;
		constexpr uint32 axis1DFlags = FKeyDetails::GamepadKey | FKeyDetails::Axis1D;
		constexpr uint32 axis2DFlags = FKeyDetails::GamepadKey | FKeyDetails::Axis2D;

		AddKey(keys.Menu, buttonFlags);
		AddKey(keys.Grip, buttonFlags);
		AddKey(keys.GripAxis, axis1DFlags);
		AddKey(keys.Trigger, buttonFlags);
		AddKey(keys.TriggerTouch, buttonFlags);
		AddKey(keys.TriggerAxis, axis1DFlags);
		AddKey(keys.Button1, buttonFlags);
		AddKey(keys.Button1Touch, buttonFlags);
		AddKey(keys.Button2, buttonFlags);
		AddKey(keys.Button2Touch, buttonFlags);
		AddKey(keys.Thumb, buttonFlags);
		AddKey(keys.ThumbTouch, buttonFlags);

		// The X and Y keys must be added before the 2D key that pairs them.
		AddKey(keys.ThumbstickX, axis1DFlags);
		AddKey(keys.ThumbstickY, axis1DFlags);
		EKeys::AddPairedKey(
			FKeyDetails(keys.Thumbstick2D.InputKey, FText::FromString(keys.Thumbstick2D.DisplayName), axis2DFlags, MenuCategory),
			keys.ThumbstickX.InputKey,
			keys.ThumbstickY.InputKey);
	}
}

FDrsMotionControllerHandKeys::FDrsMotionControllerHandKeys(EDrsHand hand)
	: Menu(MakeKey(hand, "Menu_Press", "Menu Button"))
	, Grip(MakeKey(hand, "Grip_Press", "Grip Button"))
	, GripAxis(MakeKey(hand, "Grip_Axis", "Grip Axis"))
	, Trigger(MakeKey(hand, "Trigger_Press", "Trigger Button"))
	, TriggerTouch(MakeKey(hand, "Trigger_Touch", "Trigger Touch"))
	, TriggerAxis(MakeKey(hand, "Trigger_Axis", "Trigger Axis"))
	, Button1(MakeKey(hand, "Button1_Press", "Face Button 1"))
	, Button1Touch(MakeKey(hand, "Button1_Touch", "Face Button 1 Touch"))
	, Button2(MakeKey(hand, "Button2_Press", "Face Button 2"))
	, Button2Touch(MakeKey(hand, "Button2_Touch", "Face Button 2 Touch"))
	, Thumb(MakeKey(hand, "Thumb_Press", "Thumbstick Button"))
	, ThumbTouch(MakeKey(hand, "Thumb_Touch", "Thumbstick Touch"))
	, ThumbstickX(MakeKey(hand, "Thumbstick_X", "Thumbstick X"))
	, ThumbstickY(MakeKey(hand, "Thumbstick_Y", "Thumbstick Y"))
	, Thumbstick2D(MakeKey(hand, "Thumbstick_2D", "Thumbstick 2D"))
{
}

const FDrsMotionControllerHandKeys& Drs::MotionControllerKeys::Get(EDrsHand hand)
{
	static const FDrsMotionControllerHandKeys leftKeys(EDrsHand::Left);
	static const FDrsMotionControllerHandKeys rightKeys(EDrsHand::Right);
	return (hand == EDrsHand::Left) ? leftKeys : rightKeys;
}

void Drs::MotionControllerKeys::Register()
{
	EKeys::AddMenuCategoryDisplayInfo(MenuCategory, INVTEXT("DRS Motion Controller"), TEXT("GraphEditor.PadEvent_16x"));
	RegisterHand(Get(EDrsHand::Left));
	RegisterHand(Get(EDrsHand::Right));
}

void Drs::MotionControllerKeys::Unregister()
{
	EKeys::RemoveKeysWithCategory(MenuCategory);
}
