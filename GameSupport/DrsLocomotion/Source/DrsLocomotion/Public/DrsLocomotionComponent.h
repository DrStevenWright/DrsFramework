// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DrsLocomotionComponent.generated.h"

/** Moves the owning pawn (smooth movement, teleporting, turning). Placeholder. */
UCLASS(ClassGroup = (Drs), meta = (BlueprintSpawnableComponent))
class DRSLOCOMOTION_API UDrsLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDrsLocomotionComponent();
};
