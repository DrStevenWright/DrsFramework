// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DrsPawn.generated.h"

class UCameraComponent;
class UDrsHandMeshComponent;
class UMotionControllerComponent;

/** Framework XR pawn: a floor-level tracking origin with an HMD-driven camera and two tracked hands. */
UCLASS()
class DRSCORE_API ADrsPawn : public APawn
{
	GENERATED_BODY()

public:
	ADrsPawn();

protected:
	virtual void BeginPlay() override;

	/** Floor-level origin of the tracking space. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<USceneComponent> TrackingOrigin;

	/** Camera driven by the HMD pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UCameraComponent> Camera;

	/** Left hand grip pose (controller or hand tracking, depending on the runtime). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UMotionControllerComponent> LeftController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UMotionControllerComponent> RightController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UDrsHandMeshComponent> LeftHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drs|Pawn")
	TObjectPtr<UDrsHandMeshComponent> RightHand;
};
