// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"
#include "InputTriggerFilteredPress.generated.h"

/**
 * Version of the 'Pressed' trigger that only succeeds if some minimal time has elapsed since the last time this trigger succeeded.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Pressed (Filtered)", NotInputConfigurable = "true"))
class DRSINPUT_API UInputTriggerFilteredPress : public UInputTrigger
{
	GENERATED_BODY()

public:
	
	UInputTriggerFilteredPress() {}
	
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings")
	float MinRepeatTime = 1.0f;

	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override { return ETriggerEventsSupported::Instant; }
	
	virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Explicit; }

	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override
	{
		if (IsActuated(modifiedValue) && !IsActuated(LastValue))
		{
			const double currentTriggerTime = FApp::GetCurrentTime();
			RepeatTime = (currentTriggerTime - LastTriggerTime.Get(0));
			
			if (!LastTriggerTime.IsSet() || RepeatTime >= MinRepeatTime)
			{
				LastTriggerTime = currentTriggerTime;
				return ETriggerState::Triggered;
			}
		}
		
		return ETriggerState::None;
	}

	// The engine replaces trigger instances when it rebuilds the control mappings (for example whenever any context is
	// added or removed), and only carries over the base class state.
	virtual void ReceiveTriggerReinstanced_Implementation(const UInputTrigger* oldTrigger) override
	{
		Super::ReceiveTriggerReinstanced_Implementation(oldTrigger);
		if (const UInputTriggerFilteredPress* old = Cast<const UInputTriggerFilteredPress>(oldTrigger))
		{
			LastTriggerTime = old->LastTriggerTime;
			RepeatTime = old->RepeatTime;
		}
	}

	virtual FString GetDebugState() const override
	{
		return IsActuated(LastValue) ? FString::Printf(TEXT("Pressed(Filtered):RepeatTime:%.2f/%.2f"), RepeatTime, MinRepeatTime) : FString();
	}
	
private:

	TOptional<double> LastTriggerTime;
	double RepeatTime = 0.0f;
};
