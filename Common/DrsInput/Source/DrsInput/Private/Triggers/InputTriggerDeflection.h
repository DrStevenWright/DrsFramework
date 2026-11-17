// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "DrsHand.h"
#include "DrsInputSettings.h"
#include "DrsMotionControllerKeys.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"
#include "Misc/DataValidation.h"
#include "InputTriggerDeflection.generated.h"

class UEnhancedPlayerInput;

// Allowed thumbstick directions as flags.
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDeflectionDirections : uint8
{
	None = 0 UMETA(Hidden),
	Up = (1 << 0),
	Down = (1 << 1),
	Left = (1 << 2),
	Right = (1 << 3),
	UpLeft = (1 << 4),
	UpRight = (1 << 5),
	DownLeft = (1 << 6),
	DownRight = (1 << 7),
	Default = Up UMETA(Hidden),
	
};
ENUM_CLASS_FLAGS(EDeflectionDirections)

/**
 * Input trigger that requires a controller's thumbstick to have been deflected in some specific direction (or directions).
 * This includes checking of the *initial* deflection direction, in order to capture 'deliberate' deflection inputs, rather
 * than false positives that may occur when rotating the thumbstick from one quadrant to another. (The threshold defining
 * what is considered a 'deflection' is defined in the DrsInput project settings.)
 * 
 * Deflection triggers are intended to be used in combination with other triggers in order to craft more sophisticated
 * input behaviour. In order for this to work reliably, you may need to choose the actuation threshold carefully on the
 * other triggers. In this role, one-shot "Hold" triggers can sometimes be more reliable than "Pressed" triggers (being
 * more resistant to input race conditions).
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Deflection", NotInputConfigurable = "true"))
class UInputTriggerDeflection : public UInputTrigger
{
	GENERATED_BODY()

	UInputTriggerDeflection()
	{
		ActuationThreshold = 0.0f;
	}

protected:
	
	// Specifies which thumbstick needs to be deflected to satisfy this trigger.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (DisplayThumbnail = "false"))
	EDrsHand Hand = EDrsHand::Left;
	
	// Allowed direction(s) for the thumbstick deflection.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (Bitmask, BitmaskEnum = "/Script/DrsInput.EDeflectionDirections", DisplayThumbnail = "false"))
	int32 AllowedDeflectionDirections = static_cast<int32>(EDeflectionDirections::Default);

	// Specifies whether the initial deflection direction (i.e. the direction when the thumbstick first crossed the
	// deflection threshold) must satisfy the direction requirements. Without this criterion, the trigger condition could
	// be satisfied by rotating an already-deflected thumbstick into the target direction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (DisplayThumbnail = "false"))
	bool bCheckInitialDeflectionDirection = true;

	// Specifies whether the current deflection direction must satisfy the direction requirements.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (DisplayThumbnail = "false"))
	bool bCheckCurrentDeflectionDirection = true;
	
	// Tolerated angular deviation from the allowed directions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (DisplayThumbnail = "false", Units = "deg", ClampMin = 0, ClampMax = 90))
	float AllowedAngle = 45.0f;

	// By default, the AllowedAngle applies symmetrically either side of the allowed direction. The shift property allows
	// asymmetric angle ranges to be constructed. (Positive shift value rotates the angle range clockwise and negative,
	// anti-clockwise.)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Settings", meta = (DisplayThumbnail = "false", Units = "deg", ClampMin = -45, ClampMax = 45))
	float AllowedAngleShift = 0.0f;

	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override
	{
		return ETriggerEventsSupported::Instant;
	}
	
	virtual ETriggerType GetTriggerType_Implementation() const override
	{
		return ETriggerType::Implicit;
	}
	
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override
	{
		UpdateDeflection(playerInput);
		return IsDeflectionValid() ? ETriggerState::Triggered : ETriggerState::None;
	}
	
	virtual FString GetDebugState() const override
	{
		return IsActuated(LastValue) ? (IsDeflectionValid() ? FString("Deflection:Valid") : FString("Deflection:Invalid" )) : FString();
	}
	
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override
	{
		EDataValidationResult result = Super::IsDataValid(context);

		if (AllowedDeflectionDirections == static_cast<int32>(EDeflectionDirections::None))
		{
			context.AddError(INVTEXT("Deflection trigger: No allowed directions specified."));
			result = EDataValidationResult::Invalid;
		}
		if (!bCheckInitialDeflectionDirection && !bCheckCurrentDeflectionDirection)
		{
			context.AddError(INVTEXT("Deflection trigger: Either initial or current deflection direction checks must be enabled."));
			result = EDataValidationResult::Invalid;
		}
		if (FMath::Abs(AllowedAngleShift) >= AllowedAngle)
		{
			context.AddError(INVTEXT("Deflection trigger: AllowedAngleShift must not exceed the range defined by AllowedAngle."));
			result = EDataValidationResult::Invalid;
		}
		
		return result;
	}
#endif

private:

	// Tracks the thumbstick's deflection using the device-agnostic 2D thumbstick key. That key holds the input as
	// forwarded (and possibly filtered) by the motion controller input component, so this trigger responds to exactly
	// what the rest of the game sees. Triggers are evaluated every frame, even while their own action is idle.
	void UpdateDeflection(const UEnhancedPlayerInput* playerInput)
	{
		const FKey thumbstickKey = Drs::MotionControllerKeys::Get(Hand).Thumbstick2D.InputKey;
		const FVector rawValue = playerInput ? playerInput->GetRawVectorKeyValue(thumbstickKey) : FVector::ZeroVector;
		const FVector2D thumbstickValue(rawValue.X, rawValue.Y);

		const float deflectionThreshold = UDrsInputSettings::Get().ThumbstickDeflectionThreshold;
		if (thumbstickValue.SizeSquared() <= FMath::Square(deflectionThreshold))
		{
			CurrentDeflectionDirection.Reset();
			InitialDeflectionDirection.Reset();
			return;
		}

		CurrentDeflectionDirection = thumbstickValue.GetSafeNormal();
		if (!InitialDeflectionDirection.IsSet())
		{
			InitialDeflectionDirection = CurrentDeflectionDirection;
		}
	}

	// Direction of the thumbstick when it first crossed the deflection threshold, and its direction now. Unset while
	// the thumbstick is not deflected.
	TOptional<FVector2D> InitialDeflectionDirection;
	TOptional<FVector2D> CurrentDeflectionDirection;

	bool IsDeflectionValid() const
	{
		bool checkCurrentDeflectionDirection = bCheckCurrentDeflectionDirection;
		
		if (bCheckInitialDeflectionDirection)
		{
			const TOptional<FVector2D>& deflection = InitialDeflectionDirection;
			if (!deflection.IsSet())
			{
				// If initial deflection direction has not been recorded yet, then use current direction as a temporary
				// substitute. This should help the deflection trigger activate at the same time as accompanying triggers.
				// This is particularly important for continuous triggers such as 'Pulse'.
				checkCurrentDeflectionDirection = true;
			}
			else if (!IsDeflectionValid(deflection.GetValue()))
			{
				return false;
			}
		}

		if (checkCurrentDeflectionDirection)
		{
			const TOptional<FVector2D>& deflection = CurrentDeflectionDirection;
			if (!(deflection.IsSet() && IsDeflectionValid(deflection.GetValue())))
			{
				return false;
			}
		}
		
		return true;
	}
	
	bool IsDeflectionValid(const FVector2D& deflectionDirection) const
	{
		// Identify which direction(s) the thumbstick is/was deflected in. We could do this by comparing rotation angles,
		// but this long-winded quadrant-based method is more efficient (8 comparisons and no trigonometric functions,
		// except one-off calls with cached results.)
		static const FVector2D upRightDiagonal = FVector2D(1.0, 1.0).GetSafeNormal();
		static const FVector2D upLeftDiagonal = FVector2D(-1.0, 1.0).GetSafeNormal();
		const float upRightDiagonalProjection = deflectionDirection | upRightDiagonal;
		const float upLeftDiagonalProjection = deflectionDirection | upLeftDiagonal;
		const float allowedClockwiseAngleCosine = GetAllowedAngleClockwiseCosine();
		const float allowedAntiClockwiseAngleCosine = GetAllowedAngleAntiClockwiseCosine();

		int32 deflectionDirectionFlags = 0;
		
		if (deflectionDirection.Y >= 0.f)
		{
			if (deflectionDirection.X >= 0.f)
			{
				if (deflectionDirection.Y >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Up);
				}

				if (deflectionDirection.X >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Right);
				}
			}
			else
			{
				if (deflectionDirection.Y >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Up);
				}

				if (-deflectionDirection.X >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Left);
				}
			}
		}
		else
		{
			if (deflectionDirection.X >= 0.f)
			{
				if (-deflectionDirection.Y >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Down);
				}

				if (deflectionDirection.X >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Right);
				}
			}
			else
			{
				if (-deflectionDirection.Y >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Down);
				}

				if (-deflectionDirection.X >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::Left);
				}
			}
		}

		if (upRightDiagonalProjection >= 0.f)
		{
			if (upLeftDiagonalProjection >= 0.f)
			{
				if (upRightDiagonalProjection >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::UpRight);
				}

				if (upLeftDiagonalProjection >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::UpLeft);
				}
			}
			else
			{
				if (upRightDiagonalProjection >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::UpRight);
				}

				if (-upLeftDiagonalProjection >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::DownRight);
				}
			}
		}
		else
		{
			if (upLeftDiagonalProjection >= 0.f)
			{
				if (-upRightDiagonalProjection >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::DownLeft);
				}

				if (upLeftDiagonalProjection >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::UpLeft);
				}
			}
			else
			{
				if (-upRightDiagonalProjection >= allowedAntiClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::DownLeft);
				}

				if (-upLeftDiagonalProjection >= allowedClockwiseAngleCosine)
				{
					deflectionDirectionFlags |= static_cast<int32>(EDeflectionDirections::DownRight);
				}
			}
		}
		
		// Compare deflection directions against the allowed.
		return ((deflectionDirectionFlags & AllowedDeflectionDirections) != 0);
	}
	
	float GetAllowedAngleClockwiseCosine() const
	{
		if (!AllowedClockwiseCosine.IsSet())
		{
			if (AllowedAngleShift != 0.f)
			{
				const float clampedShift = FMath::Clamp(AllowedAngleShift, -AllowedAngle, AllowedAngle);
				const float cosine = FMath::Cos(FMath::DegreesToRadians(AllowedAngle + clampedShift));
				AllowedClockwiseCosine = cosine;
			}
			else
			{
				const float cosine = FMath::Cos(FMath::DegreesToRadians(AllowedAngle));
				AllowedClockwiseCosine = cosine;
				AllowedAntiClockwiseCosine = cosine;
			}
		}
		
		return AllowedClockwiseCosine.GetValue();
	}

	float GetAllowedAngleAntiClockwiseCosine() const
	{
		if (!AllowedAntiClockwiseCosine.IsSet())
		{
			if (AllowedAngleShift != 0.f)
			{
				const float clampedShift = FMath::Clamp(AllowedAngleShift, -AllowedAngle, AllowedAngle);
				const float cosine = FMath::Cos(FMath::DegreesToRadians(AllowedAngle - clampedShift));
				AllowedAntiClockwiseCosine = cosine;
			}
			else
			{
				const float cosine = FMath::Cos(FMath::DegreesToRadians(AllowedAngle));
				AllowedClockwiseCosine = cosine;
				AllowedAntiClockwiseCosine = cosine;
			}
		}
		
		return AllowedAntiClockwiseCosine.GetValue();
	}
	
	mutable TOptional<float> AllowedClockwiseCosine;
	mutable TOptional<float> AllowedAntiClockwiseCosine;
};
