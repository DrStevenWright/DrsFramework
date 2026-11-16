// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"
#include "Misc/DataValidation.h"
#include "InputTriggerDownBuffered.generated.h"

/**
 * Version of the 'Down' input trigger with separate actuation and release thresholds, giving an input tolerance
 * buffer between starting and ending an input action.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Down (buffered)", NotInputConfigurable = "true"))
class UInputTriggerDownBuffered : public UInputTrigger
{
	GENERATED_BODY()
 
protected:
	
	// Threshold below which the trigger stops firing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (DisplayThumbnail = "false"))
	double ReleaseThreshold = 0.2;

	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override { return ETriggerEventsSupported::Instant; }
	
	virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Explicit; }
	
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override
	{
		if (bIsActuated)
		{
			if (modifiedValue.GetMagnitudeSq() >= ReleaseThreshold * ReleaseThreshold)
			{
				return ETriggerState::Triggered;
			}
			else
			{
				bIsActuated = false;
			}
		}
		else
		{
			if (modifiedValue.GetMagnitudeSq() >= ActuationThreshold * ActuationThreshold)
			{
				bIsActuated = true;
				return ETriggerState::Triggered;
			}
		}

		return ETriggerState::None;
	}

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override
	{
		EDataValidationResult result = Super::IsDataValid(context);
	
		if (ReleaseThreshold > ActuationThreshold)
		{
			context.AddError(INVTEXT("Down (buffered) trigger: Release threshold should be less than the actuation threshold."));
			result = EDataValidationResult::Invalid;
		}
	
		return result;
	}
#endif
	
private:

	bool bIsActuated = false;
};
