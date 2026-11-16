// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsInputMotionData.h"
#include "InputTriggers.h"
#include "Misc/DataValidation.h"
#include "InputTriggerControllerTracked.generated.h"

/**
 * Trigger that will fail if a specific hand has been considered 'untracked' for a certain amount of time.
 * This allows us to filter out unreliable gesture inputs.
*/
UCLASS(NotBlueprintable, meta = (DisplayName = "Controller Tracked", NotInputConfigurable = "true"))
class DRSINPUTSYSTEM_API UInputTriggerControllerTracked : public UInputTrigger
{
	GENERATED_BODY()

public:
	
	UInputTriggerControllerTracked() { ActuationThreshold = 0.0f; }
	
protected:
	
	// The motion controller (left/right) for which we should check the tracking status.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings")
	EControllerHand Hand = EControllerHand::Left;
	
	// When the hand has been untracked for longer than this time, then cancel/block this input action.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (Units = "s", ClampMin = "0.0"))
	float MaxUntrackedTime = 2.0f;
	
	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override { return ETriggerEventsSupported::Instant; }
	
	virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Implicit; }
	
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override
	{
		return IsTracked() ? ETriggerState::Triggered : ETriggerState::None;
	}

	virtual FString GetDebugState() const override
	{
		return IsActuated(LastValue) ? (IsTracked() ? FString("ControllerTracked:Tracked") : FString("UntrackedTime:Untracked" )) : FString();
	}
	
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override
	{
		EDataValidationResult result = Super::IsDataValid(context);
	
		if (Hand != EControllerHand::Left && Hand != EControllerHand::Right)
		{
			context.AddError(INVTEXT("Untracked Time trigger: Invalid hand type - must be 'Left' or 'Right'."));
			result = EDataValidationResult::Invalid;
		}
	
		return result;
	}
#endif

private:

	bool IsTracked() const
	{
		return (DrsInputMotionData::GetUntrackedTime(Hand) < MaxUntrackedTime);
	}
};
