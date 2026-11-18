// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsBase/DrsHand.h"
#include "InputCoreTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DrsHandLibrary.generated.h"

/**
 * Blueprint conversion between hand enum types. Conversions that cannot fail are autocasts, so that Blueprint inserts
 * them automatically when you connect, say, an EDrsHand pin to an EDrsHandMask pin. Conversions that can fail return
 * whether they succeeded.
 */
UCLASS()
class DRSBASEBLUEPRINT_API UDrsHandLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Converts a hand to the mask containing only that hand. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand", meta = (BlueprintAutocast, CompactNodeTitle = "->", DisplayName = "To Hand Mask (Hand)"))
	static EDrsHandMask Conv_HandToHandMask(EDrsHand hand);

	/** Converts the engine's hand enum to a hand mask. AnyHand gives Both, and every non-hand value gives None. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand", meta = (BlueprintAutocast, CompactNodeTitle = "->", DisplayName = "To Hand Mask (Controller Hand)"))
	static EDrsHandMask Conv_ControllerHandToHandMask(EControllerHand controllerHand);
	
	/** Converts a hand to the engine's hand enum. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand", meta = (BlueprintAutocast, CompactNodeTitle = "->", DisplayName = "To Controller Hand (Hand)"))
	static EControllerHand Conv_HandToControllerHand(EDrsHand hand);

	/** Gets a controller hand from a hand mask. Both and None have no engine equivalent, so this is an explicit node rather than an autocast. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand", meta = (DisplayName = "To Controller Hand (Hand Mask)"))
	static EControllerHand HandMaskToControllerHand(EDrsHandMask mask);
	
	/** Gets the hand from a mask that is exactly Left or Right. Returns false, with outHand set to Left, for None and Both. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand", meta = (DisplayName = "To Hand (Hand Mask)"))
	static bool HandMaskToHand(EDrsHandMask mask, EDrsHand& outHand);

	/** Gets the hand from the engine's hand enum. Returns false, with outHand set to Left, for AnyHand and every non-hand value. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand", meta = (DisplayName = "To Hand (Controller Hand)"))
	static bool ControllerHandToHand(EControllerHand controllerHand, EDrsHand& outHand);

	/** Gets the other hand. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand")
	static EDrsHand OppositeHand(EDrsHand hand);

	/** Whether the mask includes the hand. */
	UFUNCTION(BlueprintPure, Category = "DRS|Hand", meta = (DisplayName = "Hand Mask Contains"))
	static bool HandMaskContains(EDrsHandMask mask, EDrsHand hand);
};
