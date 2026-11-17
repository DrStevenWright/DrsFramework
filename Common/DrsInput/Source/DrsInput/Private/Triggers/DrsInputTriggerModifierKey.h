// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"
#include "DrsInputTriggerModifierKey.generated.h"

UENUM(BlueprintType)
enum class EDrsModifierKeyType : uint8
{
	// Any Shift key
	Shift,

	// Any Ctrl key
	Control,

	// Any Alt key
	Alt,
	
	LeftShift,
	RightShift,
	LeftControl,
	RightControl,
	LeftAlt,
	RightAlt,
};

/**
 * Trigger that only succeeds when a specified modifier key is pressed or not pressed.
*/
UCLASS(NotBlueprintable, meta = (DisplayName = "Modifier Key", NotInputConfigurable = "true"))
class UDrsInputTriggerModifierKey : public UInputTrigger
{
	GENERATED_BODY()

public:
	
	UDrsInputTriggerModifierKey() { ActuationThreshold = 0.0f; }
	
protected:
	
	// Modifier key that must be pressed or not pressed for this trigger to succeed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings")
	EDrsModifierKeyType ModifierKey = EDrsModifierKeyType::Shift;

	// Specifies whether the modifier key must be down or up.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings")
	bool bKeyDown = true;

	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override { return ETriggerEventsSupported::Instant; }
	
	virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Implicit; }
	
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override
	{
		bIsModifierKeyDown = false;
		// Ask the player input directly. Its outer is not guaranteed to be a player controller.
		if (const TArray<FKey>* keys = playerInput ? ModifierKeys.Find(ModifierKey) : nullptr)
		{
			for (const FKey& key : *keys)
			{
				if (playerInput->IsPressed(key))
				{
					bIsModifierKeyDown = true;
					break;
				}
			}
		}
		
		return (bKeyDown == bIsModifierKeyDown) ? ETriggerState::Triggered : ETriggerState::None;
	}

	virtual FString GetDebugState() const override
	{
		return IsActuated(LastValue) ? (bIsModifierKeyDown ? FString("ModifierKey:Down") : FString("ModifierKey:Up" )) : FString();
	}
	
private:

	bool bIsModifierKeyDown = false;
	
	inline static TMap<EDrsModifierKeyType, TArray<FKey>> ModifierKeys =
	{
		{ EDrsModifierKeyType::Shift, { EKeys::LeftShift, EKeys::RightShift } },
		{ EDrsModifierKeyType::Control, { EKeys::LeftControl, EKeys::RightControl } },
		{ EDrsModifierKeyType::Alt, { EKeys::LeftAlt, EKeys::RightAlt } },
		{ EDrsModifierKeyType::LeftShift, { EKeys::LeftShift } },
		{ EDrsModifierKeyType::RightShift, { EKeys::RightShift } },
		{ EDrsModifierKeyType::LeftControl, { EKeys::LeftControl } },
		{ EDrsModifierKeyType::RightControl, { EKeys::RightControl } },
		{ EDrsModifierKeyType::LeftAlt, { EKeys::LeftAlt } },
		{ EDrsModifierKeyType::RightAlt, { EKeys::RightAlt } },
	};
};
