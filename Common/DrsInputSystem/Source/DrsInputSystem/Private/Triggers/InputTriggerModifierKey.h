// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"
#include "InputTriggerModifierKey.generated.h"

UENUM(BlueprintType)
enum class EModifierKeyType : uint8
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
class DRSINPUTSYSTEM_API UInputTriggerModifierKey : public UInputTrigger
{
	GENERATED_BODY()

public:
	
	UInputTriggerModifierKey() { ActuationThreshold = 0.0f; }
	
protected:
	
	// Modifier key that must be pressed or not pressed for this trigger to succeed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings")
	EModifierKeyType ModifierKey = EModifierKeyType::Shift;

	// Specifies whether the modifier key must be down or up.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings")
	bool bKeyDown = true;

	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override { return ETriggerEventsSupported::Instant; }
	
	virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Implicit; }
	
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override
	{
		bIsModifierKeyDown = false;
		if (TArray<FKey>* keys = ModifierKeys.Find(ModifierKey))
		{
			for (const FKey& key : *keys)
			{
				if (playerInput->GetOuterAPlayerController()->IsInputKeyDown(key))
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
	
	inline static TMap<EModifierKeyType, TArray<FKey>> ModifierKeys =
	{
		{ EModifierKeyType::Shift, { EKeys::LeftShift, EKeys::RightShift } },
		{ EModifierKeyType::Control, { EKeys::LeftControl, EKeys::RightControl } },
		{ EModifierKeyType::Alt, { EKeys::LeftAlt, EKeys::RightAlt } },
		{ EModifierKeyType::LeftShift, { EKeys::LeftShift } },
		{ EModifierKeyType::RightShift, { EKeys::RightShift } },
		{ EModifierKeyType::LeftControl, { EKeys::LeftControl } },
		{ EModifierKeyType::RightControl, { EKeys::RightControl } },
		{ EModifierKeyType::LeftAlt, { EKeys::LeftAlt } },
		{ EModifierKeyType::RightAlt, { EKeys::RightAlt } },
	};
};
