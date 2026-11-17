// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"
#include "DrsInputTriggerDoubleTap.generated.h"

/**
 * Input trigger requiring a second input press within some interval of the first.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Double Tap", NotInputConfigurable = "true"))
class UDrsInputTriggerDoubleTap : public UInputTrigger
{
	GENERATED_BODY()
 
protected:

	// Maximum interval allowed between successive input 'taps'.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (DisplayThumbnail = "false"))
	double MaxTapInterval = 0.5;

	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override { return ETriggerEventsSupported::Instant; }
	
	virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Explicit; }
	
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override
	{
		// Time is counted from the frame deltas, so that pausing and time dilation affect it like the rest of the game.
		ElapsedTime += deltaTime;

		if (IsActuated(modifiedValue) && !IsActuated(LastValue))
		{
			if (LastTappedTime.IsSet())
			{
				LastTapInterval = ElapsedTime - LastTappedTime.GetValue();
				if (LastTapInterval < MaxTapInterval)
				{
					LastTappedTime.Reset();
					return ETriggerState::Triggered;
				}
			}
			LastTappedTime = ElapsedTime;
		}
	
		return ETriggerState::None;
	}

	// The engine replaces trigger instances when it rebuilds the control mappings (for example whenever any context is
	// added or removed), and only carries over the base class state.
	virtual void ReceiveTriggerReinstanced_Implementation(const UInputTrigger* oldTrigger) override
	{
		Super::ReceiveTriggerReinstanced_Implementation(oldTrigger);
		if (const UDrsInputTriggerDoubleTap* old = Cast<const UDrsInputTriggerDoubleTap>(oldTrigger))
		{
			ElapsedTime = old->ElapsedTime;
			LastTappedTime = old->LastTappedTime;
			LastTapInterval = old->LastTapInterval;
		}
	}

	virtual FString GetDebugState() const override
	{
		return IsActuated(LastValue) ? FString::Printf(TEXT("DoubleTap:Interval:%.2f/%.2f"), LastTapInterval, MaxTapInterval) : FString();
	}

private:
	
	double ElapsedTime = 0;
	TOptional<double> LastTappedTime;
	double LastTapInterval = 0;
};
