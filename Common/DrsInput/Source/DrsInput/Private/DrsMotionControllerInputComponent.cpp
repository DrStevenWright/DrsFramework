// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMotionControllerInputComponent.h"
#include "DrsEnhancedInputComponent.h"
#include "DrsInputMappingContext.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsMotionControllerInputComponent)

void UDrsMotionControllerInputComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	ReleaseAllForwardedKeys();
	Super::EndPlay(endPlayReason);
}

void UDrsMotionControllerInputComponent::SetHand(EDrsHand hand)
{
	Hand = hand;
}

APlayerController* UDrsMotionControllerInputComponent::GetPlayerController() const
{
	const APawn* pawn = Cast<APawn>(GetOwner());
	return pawn ? Cast<APlayerController>(pawn->GetController()) : nullptr;
}

void UDrsMotionControllerInputComponent::AddInputBindings(UDrsEnhancedInputComponent* inputComponent, const UDrsInputMappingContext* context)
{
	if (bInputBindingsAdded || !context || !ensure(inputComponent))
	{
		return;
	}

	// NOTE: If multiple input triggers are used on an action, we get a "cancelled" event rather than a "completed" one,
	// so both are treated as releases. Otherwise an input could get stuck in a pressed state.
	const FDrsMotionControllerHandKeys& keys = Drs::MotionControllerKeys::Get(Hand);

	BindButton(inputComponent, context, keys.Menu);
	BindButton(inputComponent, context, keys.Grip);
	BindButton(inputComponent, context, keys.Trigger);
	BindButton(inputComponent, context, keys.TriggerTouch);
	BindButton(inputComponent, context, keys.Button1);
	BindButton(inputComponent, context, keys.Button1Touch);
	BindButton(inputComponent, context, keys.Button2);
	BindButton(inputComponent, context, keys.Button2Touch);
	BindButton(inputComponent, context, keys.Thumb);
	BindButton(inputComponent, context, keys.ThumbTouch);

	BindAxis(inputComponent, context, keys.GripAxis);
	BindAxis(inputComponent, context, keys.TriggerAxis);
	BindAxis(inputComponent, context, keys.ThumbstickX);
	BindAxis(inputComponent, context, keys.ThumbstickY);

	bInputBindingsAdded = true;
}

void UDrsMotionControllerInputComponent::RemoveInputBindings(UDrsEnhancedInputComponent* inputComponent)
{
	if (bInputBindingsAdded && inputComponent)
	{
		inputComponent->ClearBindingsForObject(this);
	}

	bInputBindingsAdded = false;
	ReleaseAllForwardedKeys();
}

void UDrsMotionControllerInputComponent::BindButton(UDrsEnhancedInputComponent* inputComponent, const UDrsInputMappingContext* context, const FDrsMotionInputKey& key)
{
	const FDrsMotionInputKey* keyPtr = &key; // Keys have static storage duration, so the pointer stays valid.
	inputComponent->BindActionID(context, key.ActionID, ETriggerEvent::Started, this, &UDrsMotionControllerInputComponent::OnButtonStarted, keyPtr);
	inputComponent->BindActionID(context, key.ActionID, ETriggerEvent::Completed, this, &UDrsMotionControllerInputComponent::OnInputEnded, keyPtr);
	inputComponent->BindActionID(context, key.ActionID, ETriggerEvent::Canceled, this, &UDrsMotionControllerInputComponent::OnInputEnded, keyPtr);
}

void UDrsMotionControllerInputComponent::BindAxis(UDrsEnhancedInputComponent* inputComponent, const UDrsInputMappingContext* context, const FDrsMotionInputKey& key)
{
	const FDrsMotionInputKey* keyPtr = &key;
	inputComponent->BindActionID(context, key.ActionID, ETriggerEvent::Triggered, this, &UDrsMotionControllerInputComponent::OnAxisTriggered, keyPtr);
	inputComponent->BindActionID(context, key.ActionID, ETriggerEvent::Completed, this, &UDrsMotionControllerInputComponent::OnInputEnded, keyPtr);
	inputComponent->BindActionID(context, key.ActionID, ETriggerEvent::Canceled, this, &UDrsMotionControllerInputComponent::OnInputEnded, keyPtr);
}

void UDrsMotionControllerInputComponent::OnButtonStarted(const FDrsMotionInputKey* key)
{
	ForwardKeyPressed(*key, 1.0f);
}

void UDrsMotionControllerInputComponent::OnInputEnded(const FDrsMotionInputKey* key)
{
	ForwardKeyReleased(*key);
}

void UDrsMotionControllerInputComponent::OnAxisTriggered(const FInputActionValue& value, const FDrsMotionInputKey* key)
{
	ForwardKeyPressed(*key, value.Get<float>());
}

void UDrsMotionControllerInputComponent::ForwardKeyPressed(const FDrsMotionInputKey& key, float value)
{
	if (!ShouldForwardKey(key, value))
	{
		ForwardKeyReleased(key); // Make sure that filtering out an input never leaves its key stuck down.
		return;
	}

	if (APlayerController* playerController = GetPlayerController())
	{
		// If the pawn changed controller while the key was down, release it on the controller that has it down.
		const TWeakObjectPtr<APlayerController>* previous = ForwardedKeys.Find(key.InputKey);
		if (previous && previous->IsValid() && previous->Get() != playerController)
		{
			ForwardKeyReleased(key);
		}

		playerController->InputKey(FInputKeyEventArgs::CreateSimulated(key.InputKey, IE_Pressed, value));
		ForwardedKeys.Add(key.InputKey, playerController);
	}
}

void UDrsMotionControllerInputComponent::ForwardKeyReleased(const FDrsMotionInputKey& key)
{
	TWeakObjectPtr<APlayerController> playerController;
	if (ForwardedKeys.RemoveAndCopyValue(key.InputKey, playerController))
	{
		// Send the release to the controller that received the press, which the pawn may no longer have.
		if (APlayerController* controller = playerController.Get())
		{
			controller->InputKey(FInputKeyEventArgs::CreateSimulated(key.InputKey, IE_Released, 0.0f));
		}
	}
}

void UDrsMotionControllerInputComponent::ReleaseAllForwardedKeys()
{
	for (const TPair<FKey, TWeakObjectPtr<APlayerController>>& pair : ForwardedKeys)
	{
		if (APlayerController* controller = pair.Value.Get())
		{
			controller->InputKey(FInputKeyEventArgs::CreateSimulated(pair.Key, IE_Released, 0.0f));
		}
	}

	ForwardedKeys.Reset();
}
