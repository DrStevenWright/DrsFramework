// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"

/**
 * All of our DrsInput assets are contained within this plugin, and therefore the custom input triggers that those assets
 * use must be defined here too. This is a problem for triggers that rely on high-level information related to motion
 * controllers, for example. To get that information, we cannot make this plugin dependent on Game/GameSupport-plugins
 * without introducing a circular dependency. Therefore, we must allow other plugins to anonymously poke in the data needed
 * by the input trigger classes.
 */
// SWTODO - Decouple this.
namespace DrsInputMotionData
{
	DRSINPUT_API void SetThumbstickInputDirection(EControllerHand hand, const FVector2D& vector);
	DRSINPUT_API TOptional<FVector2D> GetThumbstickInitialDeflectionDirection(EControllerHand hand);
	DRSINPUT_API TOptional<FVector2D> GetThumbstickCurrentDeflectionDirection(EControllerHand hand);
}
