// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInput/Contexts/DrsEnhancedInputComponent.h"

#include "DrsInput/DrsInputLog.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsEnhancedInputComponent)

UDrsEnhancedInputComponent::FOnNotifyInputInterrupt UDrsEnhancedInputComponent::OnNotifyDanglingInput;
UDrsEnhancedInputComponent::FOnNotifyInputInterrupt UDrsEnhancedInputComponent::OnNotifyOverriddenInput;

void UDrsEnhancedInputComponent::BeginPlay()
{
	OnNotifyDanglingInput.AddUObject(this, &UDrsEnhancedInputComponent::ReleaseInterruptedInputs);
	OnNotifyOverriddenInput.AddUObject(this, &UDrsEnhancedInputComponent::ReleaseInterruptedInputs);
	
	Super::BeginPlay();
}

void UDrsEnhancedInputComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	OnNotifyDanglingInput.RemoveAll(this);
	OnNotifyOverriddenInput.RemoveAll(this);
}

void UDrsEnhancedInputComponent::ReleaseDanglingInputs(const TArray<const FInputActionInstance*>& danglingActionInstances)
{
	if (danglingActionInstances.Num() > 0)
	{
		OnNotifyDanglingInput.Broadcast(danglingActionInstances);
	}
}

void UDrsEnhancedInputComponent::ReleaseOverriddenInputs(const TArray<const FInputActionInstance*>& overriddenActionInstances)
{
	if (overriddenActionInstances.Num() > 0)
	{
		OnNotifyOverriddenInput.Broadcast(overriddenActionInstances);
	}
}

void UDrsEnhancedInputComponent::ReleaseInterruptedInputs(const TArray<const FInputActionInstance*>& interruptedActionInstances) const
{
	for (const FInputActionInstance* interruptedInstance : interruptedActionInstances)
	{
		const UInputAction* interruptedAction = interruptedInstance->GetSourceAction();
		const ETriggerEvent releaseEvent = (interruptedInstance->GetTriggerEvent() == ETriggerEvent::Triggered) ? ETriggerEvent::Completed : ETriggerEvent::Canceled;

		// Every binding for the release event needs to hear about it, not just the first, or the others stay stuck in their
		// pressed state. A handler can remove bindings (and so change the binding list) when it runs, so run copies.
		TArray<TUniquePtr<FEnhancedInputActionEventBinding>> releaseBindings;
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& binding : GetActionEventBindings())
		{
			if (binding->GetAction() == interruptedAction && binding->GetTriggerEvent() == releaseEvent)
			{
				releaseBindings.Add(binding->Clone());
			}
		}

		for (const TUniquePtr<FEnhancedInputActionEventBinding>& releaseBinding : releaseBindings)
		{
			DRS_LOG(LogDrsInput, Log, "Force executing '%s' event on input action '%s'.", *UEnum::GetValueAsString(releaseEvent), *interruptedAction->GetName());
			releaseBinding->Execute(*interruptedInstance);
		}
	}
}
