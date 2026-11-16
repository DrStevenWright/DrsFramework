// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInputMappingContext.h"
#include "DrsInputAction.h"
#include "DrsInputLog.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

void UDrsInputMappingContext::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITOR
	// Clear cached CoreInputActions when entering PIE to avoid using stale input actions.
	if (GIsEditor)
	{
		PIEBeginHandle = FEditorDelegates::BeginPIE.AddWeakLambda(this, [this](const bool isSimulating)
		{
			CoreInputActions.Empty();
		});
	}
#endif
}

void UDrsInputMappingContext::BeginDestroy()
{
#if WITH_EDITOR
	if (PIEBeginHandle.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(PIEBeginHandle);
		PIEBeginHandle.Reset();
	}
#endif
	Super::BeginDestroy();
}

const UInputAction* UDrsInputMappingContext::FindActionByName(FName actionName) const
{
	for (const FEnhancedActionKeyMapping& mapping : DefaultKeyMappings.Mappings)
	{
		if (mapping.Action && mapping.Action->GetFName() == actionName)
		{
			return mapping.Action;
		}
	}

	DRS_LOG(LogDrsInput, Log, "Could not find named InputAction '%s' in InputMappingContext '%s'", *actionName.ToString(), *GetName());
	
	return nullptr;
}

const UInputAction* UDrsInputMappingContext::FindActionByID(FName actionID) const
{
	if (CoreInputActions.IsEmpty())
	{
		for (const FEnhancedActionKeyMapping& mapping : DefaultKeyMappings.Mappings)
		{
			if (const UDrsInputAction* inputAction = Cast<UDrsInputAction>(mapping.Action))
			{
				CoreInputActions.Add(inputAction);
			}
		}
	}
	
	for (const UDrsInputAction* inputAction : CoreInputActions)
	{
		if (inputAction && inputAction->GetActionID() == actionID)
		{
			return inputAction;
		}
	}

	DRS_LOG(LogDrsInput, Log, "Could not find CoreInputAction with ID '%s' in CoreInputMappingContext '%s'", *actionID.ToString(), *GetName());
	
	return nullptr;
}

TArray<FKey> UDrsInputMappingContext::FindKeysForActionID(FName actionID) const
{
	TArray<FKey> keys;

	for (const FEnhancedActionKeyMapping& mapping : DefaultKeyMappings.Mappings)
	{
		if (const UDrsInputAction* inputAction = Cast<UDrsInputAction>(mapping.Action))
		{
			if (inputAction->GetActionID() == actionID)
			{
				keys.Add(mapping.Key);
			}
		}
	}
	
	return MoveTemp(keys);
}
