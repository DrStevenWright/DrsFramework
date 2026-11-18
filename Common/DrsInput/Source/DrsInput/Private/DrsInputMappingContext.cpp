// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInput/DrsInputMappingContext.h"

#include "DrsInput/DrsInputAction.h"
#include "DrsInput/DrsInputLog.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsInputMappingContext)

void UDrsInputMappingContext::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITOR
	// Clear cached DrsInputActions when entering PIE to avoid using stale input actions.
	if (GIsEditor)
	{
		PIEBeginHandle = FEditorDelegates::BeginPIE.AddWeakLambda(this, [this](const bool isSimulating)
		{
			DrsInputActions.Empty();
			bDrsInputActionsCached = false;
		});
	}
#endif
}

#if WITH_EDITOR
void UDrsInputMappingContext::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);
	DrsInputActions.Empty();
	bDrsInputActionsCached = false;
}
#endif

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

	DRS_LOG(LogDrsInput, Verbose, "Could not find named InputAction '%s' in InputMappingContext '%s'", *actionName.ToString(), *GetName());
	
	return nullptr;
}

const UInputAction* UDrsInputMappingContext::FindActionByID(FName actionID) const
{
	if (!bDrsInputActionsCached)
	{
		bDrsInputActionsCached = true;
		for (const FEnhancedActionKeyMapping& mapping : DefaultKeyMappings.Mappings)
		{
			if (const UDrsInputAction* inputAction = Cast<UDrsInputAction>(mapping.Action))
			{
				DrsInputActions.Add(inputAction);
			}
		}
	}
	
	for (const UDrsInputAction* inputAction : DrsInputActions)
	{
		if (inputAction && inputAction->GetActionID() == actionID)
		{
			return inputAction;
		}
	}

	DRS_LOG(LogDrsInput, Verbose, "Could not find DrsInputAction with ID '%s' in DrsInputMappingContext '%s'", *actionID.ToString(), *GetName());
	
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
