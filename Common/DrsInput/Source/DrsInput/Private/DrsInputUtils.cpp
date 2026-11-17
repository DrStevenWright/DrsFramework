// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInputUtils.h"
#include "DrsEnhancedInputComponent.h"
#include "DrsEnhancedPlayerInput.h"
#include "DrsInputLog.h"
#include "DrsInputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

UEnhancedInputLocalPlayerSubsystem* DrsInputUtils::GetEnhancedInputSubsystem(const UObject* worldContext)
{
	if (const APlayerController* controller = UGameplayStatics::GetPlayerController(worldContext, 0))
	{
		return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(controller->GetLocalPlayer());
	}
	return nullptr;
}

void DrsInputUtils::AddInputContext(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason)
{
	AddInputContext(worldContext, contextDefinition.Context, contextDefinition.Priority, reason);
}


void DrsInputUtils::RemoveInputContext(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason)
{
	RemoveInputContext(worldContext, contextDefinition.Context, reason);
}

void DrsInputUtils::AddInputContext(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EControllerHand hand, FName reason)
{
	if (hand == EControllerHand::Left)
	{
		AddInputContext(worldContext, contextDefinition.LeftHandContext, contextDefinition.Priority, reason);
	}
	else if (hand == EControllerHand::Right)
	{
		AddInputContext(worldContext, contextDefinition.RightHandContext, contextDefinition.Priority, reason);
	}
	else if (hand == EControllerHand::AnyHand)
	{
		AddInputContext(worldContext, contextDefinition.LeftHandContext, contextDefinition.Priority, reason);
		AddInputContext(worldContext, contextDefinition.RightHandContext, contextDefinition.Priority, reason);
	}
	else
	{
		DRS_LOG(LogDrsInput, Warning, "Cannot add input context definition - invalid hand.");
		ensure(false); // Help to debug which context is failing.
	}
}

void DrsInputUtils::RemoveInputContext(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EControllerHand hand, FName reason)
{
	if (hand == EControllerHand::Left)
	{
		RemoveInputContext(worldContext, contextDefinition.LeftHandContext, reason);
	}
	else if (hand == EControllerHand::Right)
	{
		RemoveInputContext(worldContext, contextDefinition.RightHandContext, reason);
	}
	else if (hand == EControllerHand::AnyHand)
	{
		RemoveInputContext(worldContext, contextDefinition.LeftHandContext, reason);
		RemoveInputContext(worldContext, contextDefinition.RightHandContext, reason);
	}
	else
	{
		DRS_LOG(LogDrsInput, Warning, "Cannot remove input context definition - invalid hand.");
		ensure(false); // Help to debug which context is failing.
	}
}

void DrsInputUtils::AddInputContextToSubsystem(UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, int32 priority, FName reason)
{
	// When an input context is added or removed, it can cause some active inputs to be left stuck in an active state.
	// (Input actions will not receive 'Completed' or 'Canceled' events. Any logic that relies on those events will be
	// broken as a result.) We therefore force complete/cancelled events on any dangling or overridden inputs.
	
	if (!ensure(IsValid(context)))
	{
		DRS_LOG(LogDrsInput, Error, "AddInputContextToSubsystem error: Could not add input context 'none' - Invalid context.");
		return;
	}
	
	if (!IsValid(enhancedInput))
	{
		DRS_LOG(LogDrsInput, Error, "AddInputContextToSubsystem error: Could not add input context '%s' - Invalid EnhancedInputLocalPlayerSubsystem.", *context->GetName());
		return;
	}
	
	int32 existingPriority = 0;
	if (!enhancedInput->HasMappingContext(context, existingPriority))
	{
		DRS_LOG(LogDrsInput, Log, "Adding input context '%s' at priority '%d'", *context->GetName(), priority);
		enhancedInput->AddMappingContext(context, priority);
		ReleaseOverriddenInputs(enhancedInput, context, priority);
	}
	else if (priority > existingPriority)
	{
		DRS_LOG(LogDrsInput, Log, "Increasing priority of input context '%s' from '%d' to '%d'", *context->GetName(), existingPriority, priority);
		enhancedInput->RemoveMappingContext(context);
		enhancedInput->AddMappingContext(context, priority);
		ReleaseDanglingInputs(enhancedInput, context);
		ReleaseOverriddenInputs(enhancedInput, context, priority);
	}
	else if (priority < existingPriority)
	{
		DRS_LOG(LogDrsInput, Log, "Decreasing priority of input context '%s' from '%d' to '%d'", *context->GetName(), existingPriority, priority);
		enhancedInput->RemoveMappingContext(context);
		enhancedInput->AddMappingContext(context, priority);
		ReleaseDanglingInputs(enhancedInput, context);
	}
	else
	{
		DRS_CLOG(UDrsEnhancedInputComponent::HasCachedInputContextReason(context, reason), LogDrsInput, Log, "Input context '%s' already added with reason '%s'.", *context->GetName(), *reason.ToString());
	}

	// Cache the reasons why we want this context added.
	UDrsEnhancedInputComponent::CacheInputContextReason(context, reason);
}

void DrsInputUtils::RemoveInputContextFromSubsystem(UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, FName reason)
{
	// When an input context is added or removed, it can cause some active inputs to be left stuck in an active state.
	// (Input actions will not receive 'Completed' or 'Canceled' events. Any logic that relies on those events will be
	// broken as a result.) We therefore force complete/cancelled events on any dangling or overridden inputs.
	
	if (!ensure(IsValid(context)))
	{
		DRS_LOG(LogDrsInput, Error, "RemoveInputContextFromSubsystem error: Could not remove input context 'none' - Invalid context.");
		return;
	}

	if (!IsValid(enhancedInput))
	{
		// This can happen legitimately during shutdown.
		DRS_LOG(LogDrsInput, Verbose, "RemoveInputContextFromSubsystem error: Could not remove input context '%s' - Invalid EnhancedInputLocalPlayerSubsystem.", *context->GetName());
		return;
	}
	
	if (!UDrsEnhancedInputComponent::UncacheInputContextReason(context, reason))
	{
		DRS_LOG(LogDrsInput, Log, "Skipping removal of input context '%s' for reasons: %s.", *context->GetName(), *UDrsEnhancedInputComponent::GetCachedInputContextReasonsString(context));
	}
	else if (enhancedInput->HasMappingContext(context))
	{
		DRS_LOG(LogDrsInput, Log, "Removing input context '%s'", *context->GetName());
		enhancedInput->RemoveMappingContext(context);
		ReleaseDanglingInputs(enhancedInput, context);
	}
	else
	{
		DRS_LOG(LogDrsInput, Log, "Input context '%s' already removed.", *context->GetName());
	}
}

bool DrsInputUtils::HasMappingForAction(const UInputMappingContext* context, const UInputAction* action)
{
	if (context && action)
	{
		for (const FEnhancedActionKeyMapping& mapping : context->GetMappings())
		{
			if (mapping.Action == action)
			{
				return true;
			}
		}
	}
	return false;
}

bool DrsInputUtils::TryGetHandForInputAction(const UInputAction* action, EControllerHand& outHand)
{
	// For actions that *might* be bound to a specific hand, this method attempts to determine which hand triggered the
	// action. For example, some actions might have an additional "non-VR" variant which would not be bound to a specific
	// hand.
	if (const UDrsInputAction* coreAction = Cast<UDrsInputAction>(action))
	{
		if (coreAction->GetHand() != EControllerHand::AnyHand)
		{
			outHand = coreAction->GetHand();
			return true;
		}
	}

	return false;
}

EControllerHand DrsInputUtils::GetHandForInputAction(const UInputAction* action, const UInputMappingContext* leftHandContext, const UInputMappingContext* rightHandContext)
{
	// For actions that are *expected* to be bound to a specific hand, this method attempts to determine which hand
	// triggered the action. In such cases, actions would typically be mapped in separate left- and right-hand input
	// contexts. So we can use that as a fallback criteria if no hand type is specified on the action itself.
	if (const UDrsInputAction* coreAction = Cast<UDrsInputAction>(action))
	{
		if (coreAction->GetHand() != EControllerHand::AnyHand)
		{
			return coreAction->GetHand();
		}
	}
	
	const bool isMappedToLeftHand = HasMappingForAction(leftHandContext, action);
	const bool isMappedToRightHand = HasMappingForAction(rightHandContext, action);
	
	if (isMappedToLeftHand && !isMappedToRightHand)
	{
		return EControllerHand::Left;
	}

	if (isMappedToRightHand && !isMappedToLeftHand)
	{
		return EControllerHand::Right;
	}

	DRS_LOG(LogDrsInput, Warning, "Could not determine hand type for input action '%s'", action ? *action->GetName() : TEXT("None"));
	
	return EControllerHand::AnyHand;
}

void DrsInputUtils::ClearBindingsForContext(UEnhancedInputComponent* inputComponent, const UObject* owner, const UInputMappingContext* context)
{
	if (inputComponent == nullptr || owner == nullptr || context == nullptr)
	{
		return;
	}
	
	const auto& bindings = inputComponent->GetActionEventBindings();
	for (int32 index = bindings.Num() - 1; index >= 0; --index)
	{
		if (bindings[index]->IsBoundToObject(owner) && HasMappingForAction(context, bindings[index]->GetAction()))
		{
			inputComponent->RemoveActionEventBinding(index);
		}
	}
}

void DrsInputUtils::ClearBindingsForHand(UEnhancedInputComponent* inputComponent, UObject* owner, EControllerHand hand)
{
	if (inputComponent == nullptr || owner == nullptr)
	{
		return;
	}

	if (hand == EControllerHand::AnyHand)
	{
		inputComponent->ClearBindingsForObject(owner);
		return;
	}
	
	const auto& bindings = inputComponent->GetActionEventBindings();
	for (int32 index = bindings.Num() - 1; index >= 0; --index)
	{
		if (const UDrsInputAction* action = Cast<UDrsInputAction>(bindings[index]->GetAction()))
		{
			if (action->GetHand() == hand && bindings[index]->IsBoundToObject(owner))
			{
				inputComponent->RemoveActionEventBinding(index);
			}
		}
	}
}

void DrsInputUtils::ReleaseDanglingInputs(const UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context)
{
	if (UDrsEnhancedPlayerInput* playerInput = enhancedInput ? Cast<UDrsEnhancedPlayerInput>(enhancedInput->GetPlayerInput()) : nullptr)
	{
		// Delay required to prevent stray Triggered events after the dangling action(s) have been completed.
		playerInput->GetWorld()->GetTimerManager().SetTimerForNextTick([weakPlayerInput = MakeWeakObjectPtr(playerInput), weakContext = MakeWeakObjectPtr(context)]()
		{
			UDrsEnhancedPlayerInput* playerInput = weakPlayerInput.Get();
			if (IsValid(playerInput))
			{
				playerInput->ReleaseDanglingInputs(weakContext.Get());
			}
		});
	}
}

void DrsInputUtils::ReleaseOverriddenInputs(const UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, int32 priority)
{
	// Sometimes UE handles this case natively and sometimes it doesn't. :-/
	if (UDrsEnhancedPlayerInput* playerInput = enhancedInput ? Cast<UDrsEnhancedPlayerInput>(enhancedInput->GetPlayerInput()) : nullptr)
	{
		playerInput->ReleaseOverriddenInputs(context, priority);
	}
}

void DrsInputUtils::FlushPressedKeys(const UObject* worldContext)
{
	const UEnhancedInputLocalPlayerSubsystem* enhancedInputSubsystem = GetEnhancedInputSubsystem(worldContext);
	if (UEnhancedPlayerInput* playerInput = enhancedInputSubsystem ? enhancedInputSubsystem->GetPlayerInput() : nullptr)
	{
		playerInput->FlushPressedKeys();
	}
}
