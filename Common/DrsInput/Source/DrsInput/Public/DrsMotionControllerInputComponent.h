// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DrsHand.h"
#include "DrsMotionControllerKeys.h"
#include "DrsMotionControllerInputComponent.generated.h"

class APlayerController;
class UDrsEnhancedInputComponent;
class UDrsInputMappingContext;
struct FInputActionValue;

/**
 * Forwards one hand's motion controller input as device-agnostic input keys (see FDrsMotionControllerHandKeys).
 *
 * A global input mapping context binds every supported device's physical inputs to input actions with the matching action
 * IDs. This component binds to those actions and re-issues them as simulated key events on the player controller, so that
 * all other input mapping contexts only ever need to reference the device-agnostic keys.
 *
 * Derive from this class to filter which inputs get forwarded (override ShouldForwardKey) or to add further bindings.
 * Platform-specific behaviour such as controller capabilities, hand tracking or emulated inputs belongs in a derived class
 * in a higher layer.
 */
UCLASS(BlueprintType, ClassGroup = (Input))
class DRSINPUT_API UDrsMotionControllerInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/** Sets the hand that this component forwards input for. Call before adding input bindings. */
	void SetHand(EDrsHand hand);

	UFUNCTION(BlueprintPure, Category = "DRS|Input")
	EDrsHand GetHand() const { return Hand; }

	/**
	 * Binds to the motion controller actions (found by action ID) in the given context.
	 * Any forwarded input that is no longer wanted is released in response to RemoveInputBindings or EndPlay.
	 */
	void AddInputBindings(UDrsEnhancedInputComponent* inputComponent, const UDrsInputMappingContext* context);
	void RemoveInputBindings(UDrsEnhancedInputComponent* inputComponent);
	bool AreInputBindingsAdded() const { return bInputBindingsAdded; }

	/** Releases every key that is currently being forwarded as pressed. */
	void ReleaseAllForwardedKeys();

protected:

	/** Gets the player controller that forwarded keys are sent to. Defaults to the owning pawn's controller. */
	virtual APlayerController* GetPlayerController() const;

	/**
	 * Decides whether an input is forwarded as a device-agnostic key. Override to filter inputs. If this returns false
	 * for a key that is currently being forwarded as pressed, the key is released, so that filtering can never leave a
	 * key stuck down. Releases that arrive from the device are always forwarded.
	 * @param key	The device-agnostic key that would be pressed.
	 * @param value	The value that would be forwarded (1 for buttons).
	 */
	virtual bool ShouldForwardKey(const FDrsMotionInputKey& key, float value) const { return true; }

	/** Forwards a press, or an analogue update, on the key (subject to ShouldForwardKey). */
	void ForwardKeyPressed(const FDrsMotionInputKey& key, float value);

	/** Forwards a release on the key, if the key is currently being forwarded as pressed. */
	void ForwardKeyReleased(const FDrsMotionInputKey& key);

private:

	void OnButtonStarted(const FDrsMotionInputKey* key);
	void OnInputEnded(const FDrsMotionInputKey* key);
	void OnAxisTriggered(const FInputActionValue& value, const FDrsMotionInputKey* key);

	void BindButton(UDrsEnhancedInputComponent* inputComponent, const UDrsInputMappingContext* context, const FDrsMotionInputKey& key);
	void BindAxis(UDrsEnhancedInputComponent* inputComponent, const UDrsInputMappingContext* context, const FDrsMotionInputKey& key);

	// Hand this component forwards input for.
	UPROPERTY(EditAnywhere, BlueprintGetter = GetHand, Category = "DRS|Input")
	EDrsHand Hand = EDrsHand::Left;

	// Keys that are currently being forwarded as pressed, and the controller that each press was sent to. The release has
	// to go to that same controller, because by the time the pawn is destroyed or unpossessed it no longer has one.
	TMap<FKey, TWeakObjectPtr<APlayerController>> ForwardedKeys;

	bool bInputBindingsAdded = false;
};
