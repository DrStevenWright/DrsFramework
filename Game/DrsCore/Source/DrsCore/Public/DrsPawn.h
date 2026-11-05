// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DrsPawn.generated.h"

class UCameraComponent;
class UDrsInteractorComponent;
class UDrsLocomotionComponent;

/** Framework XR pawn: a tracking origin with an HMD-driven camera. */
UCLASS()
class DRSCORE_API ADrsPawn : public APawn
{
	GENERATED_BODY()

public:
	ADrsPawn();

protected:
	/** Floor-level origin of the tracking space. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<USceneComponent> TrackingOrigin;

	/** Camera driven by the HMD pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UDrsInteractorComponent> Interactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UDrsLocomotionComponent> Locomotion;
};
