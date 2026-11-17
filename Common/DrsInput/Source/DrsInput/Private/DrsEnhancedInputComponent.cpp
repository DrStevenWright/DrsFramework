// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsEnhancedInputComponent.h"
#include "DrsInputLog.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsEnhancedInputComponent)

UDrsEnhancedInputComponent::FOnNotifyInputInterrupt UDrsEnhancedInputComponent::OnNotifyDanglingInput;
UDrsEnhancedInputComponent::FOnNotifyInputInterrupt UDrsEnhancedInputComponent::OnNotifyOverriddenInput;

TMap<FName, TSet<FName>> UDrsEnhancedInputComponent::ActiveInputContexts;

void UDrsEnhancedInputComponent::BeginPlay()
{
	OnNotifyDanglingInput.AddUObject(this, &UDrsEnhancedInputComponent::ReleaseInterruptedInputs);
	OnNotifyOverriddenInput.AddUObject(this, &UDrsEnhancedInputComponent::ReleaseInterruptedInputs);
	
	Super::BeginPlay();
}

void UDrsEnhancedInputComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	ActiveInputContexts.Reset();
	
	Super::EndPlay(endPlayReason);

	OnNotifyDanglingInput.RemoveAll(this);
	OnNotifyOverriddenInput.RemoveAll(this);
}

void UDrsEnhancedInputComponent::CacheInputContextReason(const UInputMappingContext* context, const FName& reason)
{
	if (context)
	{
		TSet<FName>& reasons = ActiveInputContexts.FindOrAdd(context->GetFName());
		reasons.Add(reason);
	}
}

bool UDrsEnhancedInputComponent::UncacheInputContextReason(const UInputMappingContext* context, const FName& reason)
{
	if (TSet<FName>* reasons = context ? ActiveInputContexts.Find(context->GetFName()) : nullptr)
	{
		reasons->Remove(reason);
		if (reasons->Num() == 0)
		{
			ActiveInputContexts.Remove(context->GetFName());
			return true;
		}
		return false;
	}
	else
	{
		return true;
	}
}

bool UDrsEnhancedInputComponent::HasCachedInputContextReason(const UInputMappingContext* context, const FName& reason)
{
	if (const TSet<FName>* reasons = context ? ActiveInputContexts.Find(context->GetFName()) : nullptr)
	{
		return reasons->Contains(reason);
	}
	return false;
}

TArray<FName> UDrsEnhancedInputComponent::GetCachedInputContexts()
{
	TArray<FName> keys;
	ActiveInputContexts.GetKeys(keys);
	return keys;
}

TArray<FName> UDrsEnhancedInputComponent::GetCachedInputContextReasons(const UInputMappingContext* context)
{
	if (const TSet<FName>* reasons = context ? ActiveInputContexts.Find(context->GetFName()) : nullptr)
	{
		return reasons->Array();
	}
	else
	{
		return {};
	}
}

FString UDrsEnhancedInputComponent::GetCachedInputContextReasonsString(const UInputMappingContext* context)
{
	FString reasonsString;
	
	const TArray<FName> reasons = GetCachedInputContextReasons(context);
	if (reasons.Num() > 0)
	{
		reasonsString += reasons[0].ToString();
		for (int32 i = 1; i < reasons.Num(); i++)
		{
			reasonsString += ", ";
			reasonsString += reasons[i].ToString();
		}
	}

	return reasonsString;
}

void UDrsEnhancedInputComponent::PrintCachedInputContextReasons(const UInputMappingContext* context)
{
	if (context)
	{
		if (ActiveInputContexts.Find(context->GetFName()))
		{
			DRS_LOG(LogDrsInput, Log, "Input context '%s' was added with the following specified reasons: %s.", *context->GetName(), *GetCachedInputContextReasonsString(context));
		}
		else
		{
			DRS_LOG(LogDrsInput, Log, "Input context '%s' was not added with any specified reasons.", *context->GetName());
		}
	}
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

		if (auto* binding = GetActionEventBindings().FindByPredicate([interruptedAction, releaseEvent](const auto& currentBinding)
			{
				return (currentBinding->GetAction() == interruptedAction && currentBinding->GetTriggerEvent() == releaseEvent);
			})
		)
		{
			DRS_LOG(LogDrsInput, Log, "Force executing '%s' event on input action '%s'.", *UEnum::GetValueAsString(releaseEvent), *interruptedAction->GetName());
			(*binding)->Execute(*interruptedInstance);
		}
	}
}
