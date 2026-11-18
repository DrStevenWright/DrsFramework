// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsBase/DrsHand.h"

#include "InputCoreTypes.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsHand)

TOptional<EDrsHand> Drs::ToHand(EControllerHand controllerHand)
{
	if (controllerHand == EControllerHand::Left)
	{
		return EDrsHand::Left;
	}
	if (controllerHand == EControllerHand::Right)
	{
		return EDrsHand::Right;
	}
	return TOptional<EDrsHand>();
}

EDrsHandMask Drs::ToMask(EControllerHand controllerHand)
{
	switch (controllerHand)
	{
	case EControllerHand::Left:
		return EDrsHandMask::Left;
	case EControllerHand::Right:
		return EDrsHandMask::Right;
	case EControllerHand::AnyHand:
		return EDrsHandMask::Both;
	default:
		return EDrsHandMask::None;
	}
}

EControllerHand Drs::ToControllerHand(EDrsHand hand)
{
	return (hand == EDrsHand::Left) ? EControllerHand::Left : EControllerHand::Right;
}

EControllerHand Drs::ToControllerHand(EDrsHandMask mask)
{
	if (mask == EDrsHandMask::Left)
	{
		return EControllerHand::Left;
	}
	if (mask == EDrsHandMask::Right)
	{
		return EControllerHand::Right;
	}
	return EControllerHand::AnyHand;
}

