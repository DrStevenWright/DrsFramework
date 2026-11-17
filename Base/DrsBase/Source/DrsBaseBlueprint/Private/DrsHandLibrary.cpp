// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsHandLibrary.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsHandLibrary)

EDrsHandMask UDrsHandLibrary::Conv_HandToHandMask(EDrsHand hand)
{
	return Drs::ToMask(hand);
}

EDrsHandMask UDrsHandLibrary::Conv_ControllerHandToHandMask(EControllerHand controllerHand)
{
	return Drs::ToMask(controllerHand);
}

EControllerHand UDrsHandLibrary::Conv_HandToControllerHand(EDrsHand hand)
{
	return Drs::ToControllerHand(hand);
}

EControllerHand UDrsHandLibrary::HandMaskToControllerHand(EDrsHandMask mask)
{
	return Drs::ToControllerHand(mask);
}

bool UDrsHandLibrary::HandMaskToHand(EDrsHandMask mask, EDrsHand& outHand)
{
	const TOptional<EDrsHand> hand = Drs::ToHand(mask);
	outHand = hand.Get(EDrsHand::Left); // Blueprint output pins should always be assigned.
	return hand.IsSet();
}

bool UDrsHandLibrary::ControllerHandToHand(EControllerHand controllerHand, EDrsHand& outHand)
{
	const TOptional<EDrsHand> hand = Drs::ToHand(controllerHand);
	outHand = hand.Get(EDrsHand::Left);
	return hand.IsSet();
}

EDrsHand UDrsHandLibrary::OppositeHand(EDrsHand hand)
{
	return Drs::Opposite(hand);
}

bool UDrsHandLibrary::HandMaskContains(EDrsHandMask mask, EDrsHand hand)
{
	return Drs::Contains(mask, hand);
}
