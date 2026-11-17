// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DrsPawn.generated.h"

class UCameraComponent;
class UDrsHandMeshComponent;
class UDrsInputMappingContext;
class UDrsMotionControllerInputComponent;
class UInputComponent;
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
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* playerInputComponent) override;
	virtual void UnPossessed() override;

	/** Adds or removes MotionControllerContext on the local player that controls this pawn. */
	void AddMotionControllerContext();
	void RemoveMotionControllerContext();

	/** Floor-level origin of the tracking space. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<USceneComponent> TrackingOrigin;

	/** Camera driven by the HMD pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<UCameraComponent> Camera;

	/** Left hand grip pose (controller or hand tracking, depending on the runtime). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<UMotionControllerComponent> LeftController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<UMotionControllerComponent> RightController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<UDrsHandMeshComponent> LeftHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<UDrsHandMeshComponent> RightHand;

	/** Forwards each hand's motion controller input as device-agnostic input keys. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<UDrsMotionControllerInputComponent> LeftMotionInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<UDrsMotionControllerInputComponent> RightMotionInput;

	/**
	 * Maps each platform's physical motion controller inputs to the input actions that the motion input components forward
	 * as device-agnostic keys. It is added to the controlling local player at the highest possible priority so that nothing can
	 * override it. The project's default input component class must be UDrsEnhancedInputComponent.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DRS|Pawn")
	TObjectPtr<const UDrsInputMappingContext> MotionControllerContext;

	// Whether MotionControllerContext is currently added to the controlling local player.
	bool bMotionControllerContextAdded = false;
};
