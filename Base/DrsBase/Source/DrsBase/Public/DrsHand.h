// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"
#include "DrsHand.generated.h"

enum class EControllerHand : uint8;

/**
 * Which of the local user's hands something represents. Always exactly one hand, so if it is not Left then it is Right.
 * Use EDrsHandMask where "no hand" or "both hands" must also be expressible.
 */
UENUM(BlueprintType)
enum class EDrsHand : uint8
{
	Left,
	Right
};

/** A set of hands. Use this to select which hand(s) something applies to. */
UENUM(BlueprintType)
enum class EDrsHandMask : uint8
{
	None = 0,
	Left = 1,
	Right = 2,
	Both = 3
};
ENUM_CLASS_FLAGS(EDrsHandMask)

namespace Drs
{
	constexpr EDrsHand Opposite(EDrsHand hand)
	{
		return (hand == EDrsHand::Left) ? EDrsHand::Right : EDrsHand::Left;
	}

	/** Converts from the DRS hand enum to hand mask. Always returns a value. */
	constexpr EDrsHandMask ToMask(EDrsHand hand)
	{
		return (hand == EDrsHand::Left) ? EDrsHandMask::Left : EDrsHandMask::Right;
	}

	/** Whether a hand mask includes a specific hand. */
	constexpr bool Contains(EDrsHandMask mask, EDrsHand hand)
	{
		return (static_cast<uint8>(mask) & static_cast<uint8>(ToMask(hand))) != 0;
	}
	
	/** Converts from DRS hand mask to explicit hand enum. Unset for None and Both. */
	FORCEINLINE TOptional<EDrsHand> ToHand(EDrsHandMask mask)
	{
		if (mask == EDrsHandMask::Left)
		{
			return EDrsHand::Left;
		}
		if (mask == EDrsHandMask::Right)
		{
			return EDrsHand::Right;
		}
		return TOptional<EDrsHand>();
	}

	/** Converts from the engine's hand enum to explicit hand enum. Unset for AnyHand and every non-hand value. */
	DRSBASE_API TOptional<EDrsHand> ToHand(EControllerHand controllerHand);
	
	/** Converts from the engine's hand enum to DRS hand mask. AnyHand gives Both. Every non-hand value (Gun, Pad and so on) gives None. */
	DRSBASE_API EDrsHandMask ToMask(EControllerHand controllerHand);
	
	/** Converts from DRS hand enum to the engine's hand enum. */
	DRSBASE_API EControllerHand ToControllerHand(EDrsHand hand);

	/** Converts from DRS hand mask to the engine's hand enum. Both and None have no engine equivalent other than AnyHand, so both give AnyHand. */
	DRSBASE_API EControllerHand ToControllerHand(EDrsHandMask mask);
}
