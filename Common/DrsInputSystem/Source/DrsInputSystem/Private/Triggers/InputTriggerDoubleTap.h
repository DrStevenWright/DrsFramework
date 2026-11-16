// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"
#include "InputTriggerDoubleTap.generated.h"

/**
 * Input trigger requiring a second input press within some interval of the first.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Double Tap", NotInputConfigurable = "true"))
class UInputTriggerDoubleTap : public UInputTrigger
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
		if (IsActuated(modifiedValue) && !IsActuated(LastValue))
		{
			const double currentTime = FApp::GetCurrentTime();
			const double currentTapInterval = currentTime - LastTappedTime;
			if (currentTapInterval < MaxTapInterval)
			{
				LastTappedTime = 0;
				return ETriggerState::Triggered;
			}
			LastTappedTime = currentTime;
			LastTapInterval = currentTapInterval;
		}
	
		return ETriggerState::None;
	}

	virtual FString GetDebugState() const override
	{
		return IsActuated(LastValue) ? FString::Printf(TEXT("DoubleTap:Interval:%.2f/%.2f"), LastTapInterval, MaxTapInterval) : FString();
	}

private:
	
	double LastTappedTime = 0;
	double LastTapInterval = 0;
};
