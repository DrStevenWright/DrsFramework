// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "InputModifiers.h"
#include "DrsInputModifierScaleByClampedDeltaTime.generated.h"

/**
 * Clamped version of the 'Scale By Delta Time' modifier. This multiplies the input value by the current frame's time
 * delta, up to some specified maximum value (in case of performance spikes).
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Scale By Delta Time (Clamped)"))
class UDrsInputModifierScaleByClampedDeltaTime : public UInputModifier
{
	GENERATED_BODY()
	
protected:

	// Maximum time-delta (in seconds) to clamp against.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier Settings", meta = (DisplayThumbnail = "false"))
	float MaxDelta = 0.25f;
	
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
	{
		if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Scale By Delta Time modifier doesn't support boolean values.")))
		{
			return currentValue.Get<FVector>() * FMath::Min(deltaTime, MaxDelta);
		}
		return currentValue;
	}
};
