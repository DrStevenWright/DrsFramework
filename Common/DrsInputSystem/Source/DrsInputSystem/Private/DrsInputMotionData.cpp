// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInputMotionData.h"
#include "DrsInputSettings.h"

namespace
{
	TOptional<FVector2D> LeftThumbstickInitialDeflectionDirection;
	TOptional<FVector2D> RightThumbstickInitialDeflectionDirection;
	TOptional<FVector2D> LeftThumbstickCurrentDeflectionDirection;
	TOptional<FVector2D> RightThumbstickCurrentDeflectionDirection;
}

void DrsInputMotionData::SetThumbstickInputDirection(EControllerHand hand, const FVector2D& vector)
{
	const float deflectionThreshold = UDrsInputSettings::Get().ThumbstickDeflectionThreshold;
	const bool deflected = vector.SizeSquared() > (deflectionThreshold * deflectionThreshold);
		
	switch (hand)
	{
	case EControllerHand::Left:
		if (!deflected)
		{
			LeftThumbstickCurrentDeflectionDirection.Reset();
			LeftThumbstickInitialDeflectionDirection.Reset();
		}
		else
		{
			LeftThumbstickCurrentDeflectionDirection =  vector.GetSafeNormal();
			if (!LeftThumbstickInitialDeflectionDirection.IsSet())
			{
				LeftThumbstickInitialDeflectionDirection = LeftThumbstickCurrentDeflectionDirection.GetValue();
			}
		}
		break;

	case EControllerHand::Right:
		if (!deflected)
		{
			RightThumbstickCurrentDeflectionDirection.Reset();
			RightThumbstickInitialDeflectionDirection.Reset();
		}
		else
		{
			RightThumbstickCurrentDeflectionDirection = vector.GetSafeNormal();
			if (!RightThumbstickInitialDeflectionDirection.IsSet())
			{
				RightThumbstickInitialDeflectionDirection = RightThumbstickCurrentDeflectionDirection.GetValue();
			}
		}
		break;
		
	default:
		break;
	}
}

TOptional<FVector2D> DrsInputMotionData::GetThumbstickCurrentDeflectionDirection(EControllerHand hand)
{
	switch (hand)
	{
	case EControllerHand::Left:
		return LeftThumbstickCurrentDeflectionDirection;

	case EControllerHand::Right:
		return RightThumbstickCurrentDeflectionDirection;
		
	default:
		break;
	}
	
	return TOptional<FVector2D>();
}

TOptional<FVector2D> DrsInputMotionData::GetThumbstickInitialDeflectionDirection(EControllerHand hand)
{
	switch (hand)
	{
	case EControllerHand::Left:
		return LeftThumbstickInitialDeflectionDirection;

	case EControllerHand::Right:
		return RightThumbstickInitialDeflectionDirection;
		
	default:
		break;
	}
	
	return TOptional<FVector2D>();
}

