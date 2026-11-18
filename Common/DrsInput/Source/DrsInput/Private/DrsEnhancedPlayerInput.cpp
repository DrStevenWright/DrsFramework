// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInput/DrsEnhancedPlayerInput.h"

#include "DrsInput/DrsEnhancedInputComponent.h"
#include "DrsInput/DrsInputLog.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsEnhancedPlayerInput)

void UDrsEnhancedPlayerInput::ReleaseDanglingInputs(const UInputMappingContext* contextBeingRemoved) const
{
	if (!IsValid(contextBeingRemoved))
	{
		return;
	}
	
	TArray<const FInputActionInstance*> danglingActions;
	for (const auto& mapping : contextBeingRemoved->GetMappings())
	{
		if (const FInputActionInstance* instance = IsValid(mapping.Action) ? FindActionInstanceData(mapping.Action) : nullptr)
		{
			switch (instance->GetTriggerEvent())
			{
			case ETriggerEvent::Started:
			case ETriggerEvent::Ongoing:
			case ETriggerEvent::Triggered:
				if (!danglingActions.Contains(instance))
				{
					DRS_LOG(LogDrsInput, Log, "Releasing input action '%s' dangling from context '%s'.", *mapping.Action->GetName(), *contextBeingRemoved->GetName());
					danglingActions.Add(instance);
				}
				break;

			default:
				break;
			}
		}
	}
	
	if (danglingActions.Num() > 0)
	{
		UDrsEnhancedInputComponent::ReleaseDanglingInputs(danglingActions);
	} 
}

void UDrsEnhancedPlayerInput::ReleaseOverriddenInputs(const UInputMappingContext* contextBeingAdded, int32 priority)
{
	if (contextBeingAdded == nullptr)
	{
		return;
	}
	
	TArray<FKey> overriddenKeys;
	for (const auto& mapping : contextBeingAdded->GetMappings())
	{
		if (const bool* keydown = KeyDownPrevious.Find(mapping.Key))
		{
			// A row without an action (easy to leave while editing a context) consumes nothing.
			if (*keydown && IsValid(mapping.Action) && mapping.Action->bConsumeInput)
			{
				overriddenKeys.Emplace(mapping.Key);
			}
		}
	}
	
	if (overriddenKeys.Num() == 0)
	{
		return;
	}
	
	TArray<const FInputActionInstance*> overriddenActions;
	for (const auto& pair : GetAppliedInputContextData())
	{
		if (pair.Value.Priority < priority && pair.Key != contextBeingAdded)
		{
			const UInputMappingContext* overriddenContext = pair.Key;
			for (const FKey& overriddenKey : overriddenKeys)
			{
				// Several actions in the context can be mapped to the same key, and each one is overridden.
				for (const FEnhancedActionKeyMapping& overriddenMapping : overriddenContext->GetMappings())
				{
					if (overriddenMapping.Key != overriddenKey || !IsValid(overriddenMapping.Action))
					{
						continue;
					}

					if (const FInputActionInstance* instance = FindActionInstanceData(overriddenMapping.Action))
					{
						switch (instance->GetTriggerEvent())
						{
						case ETriggerEvent::Started:
						case ETriggerEvent::Ongoing:
						case ETriggerEvent::Triggered:
							if (!overriddenActions.Contains(instance))
							{
								// TODO: In some extreme edge cases we cannot be 100% certain that the input key being
								// consumed is the one actually triggering the action.
								DRS_LOG(LogDrsInput, Log, "Releasing input action '%s' in context '%s', overridden by key '%s' in context '%s'", *overriddenMapping.Action->GetName(), *overriddenContext->GetName(), *overriddenKey.ToString(), *contextBeingAdded->GetName());
								overriddenActions.Add(instance);
							}
							break;

						default:
							break;
						}
					}
				}
			}
		}
	}

	if (overriddenActions.Num() > 0)
	{
		UDrsEnhancedInputComponent::ReleaseOverriddenInputs(overriddenActions);
	} 
}
