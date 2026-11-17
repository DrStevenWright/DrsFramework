// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsHand.h"
#include "InputAction.h"
#include "DrsInputAction.generated.h"

/** Custom class for input actions used by Drs. */
UCLASS(BlueprintType)
class DRSINPUT_API UDrsInputAction : public UInputAction
{
	GENERATED_BODY()

public:

	UDrsInputAction()
	{
		ActionDescription = FText::AsCultureInvariant(ActionDescription);
	}
	
	FName GetActionID() const { return ActionID; }

	EDrsHandMask GetHands() const { return Hands; }

protected:

	// Custom ID to allow Drs code to reference input action assets without needing to expose them as properties and
	// without being coupled to a specific asset name.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Drs|Input")
	FName ActionID = NAME_None;

	// Optionally tag the hand(s) that this action is associated with. None means that the action is not hand-specific.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Drs|Input")
	EDrsHandMask Hands = EDrsHandMask::None;
};
