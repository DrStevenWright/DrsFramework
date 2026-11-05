// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DrsInteractorComponent.generated.h"

/** Lets the owning pawn interact with interactable objects. Placeholder. */
UCLASS(ClassGroup = (Drs), meta = (BlueprintSpawnableComponent))
class DRSINTERACTION_API UDrsInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDrsInteractorComponent();
};
