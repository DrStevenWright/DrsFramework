// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "DrsInputAction.generated.h"

/** Custom class for input actions used by Drs. */
UCLASS(BlueprintType)
class DRSINPUTSYSTEM_API UDrsInputAction : public UInputAction
{
	GENERATED_BODY()

public:

	UDrsInputAction()
	{
		ActionDescription = FText::AsCultureInvariant(ActionDescription);
	}
	
	FName GetActionID() const { return ActionID; }

	EControllerHand GetHand() const { return Hand; }

protected:

	// Custom ID to allow Core code to reference input action assets without needing to expose them as properties and
	// without being coupled to a specific asset name.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FName ActionID = NAME_None;

	// Optionally tag that this action is associated with a specific hand.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	EControllerHand Hand = EControllerHand::AnyHand;
};
