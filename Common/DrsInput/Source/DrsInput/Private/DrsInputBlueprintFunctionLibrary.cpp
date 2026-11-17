// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInputBlueprintFunctionLibrary.h"
#include "DrsInputSettings.h"
#include "DrsInputUtils.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsInputBlueprintFunctionLibrary)

namespace
{
	TSet<FName> ActiveCustomInputs[static_cast<int32>(ECustomInputKeyScope::NumScopes)]; // Custom input keys pressed within each scope.

	TSet<FName>& GetActiveCustomInputs(ECustomInputKeyScope scope)
	{
		check(scope != ECustomInputKeyScope::NumScopes);
		return ActiveCustomInputs[static_cast<uint8>(scope)];
	}
}

const UDrsInputSettings* UDrsInputBlueprintFunctionLibrary::GetDrsInputSettings()
{
	return &UDrsInputSettings::Get();
}

void UDrsInputBlueprintFunctionLibrary::AddInputContext(const UObject* worldContext, const UInputMappingContext* context, int32 priority, FName reason)
{
	DrsInputUtils::AddInputContext(worldContext, context, priority, reason);
}

void UDrsInputBlueprintFunctionLibrary::RemoveInputContext(const UObject* worldContext, const UInputMappingContext* context, FName reason)
{
	DrsInputUtils::RemoveInputContext(worldContext, context, reason);
}

void UDrsInputBlueprintFunctionLibrary::AddInputContextDefinition(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason)
{
	DrsInputUtils::AddInputContext(worldContext, contextDefinition, reason);
}

void UDrsInputBlueprintFunctionLibrary::RemoveInputContextDefinition(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason)
{
	DrsInputUtils::RemoveInputContext(worldContext, contextDefinition, reason);
}

void UDrsInputBlueprintFunctionLibrary::AddInputContextDefinitionForHands(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EDrsHandMask hands, FName reason)
{
	DrsInputUtils::AddInputContext(worldContext, contextDefinition, hands, reason);
}

void UDrsInputBlueprintFunctionLibrary::RemoveInputContextDefinitionForHands(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EDrsHandMask hands, FName reason)
{
	DrsInputUtils::RemoveInputContext(worldContext, contextDefinition, hands, reason);
}

void UDrsInputBlueprintFunctionLibrary::FlushPressedKeys(const UObject* worldContext)
{
	DrsInputUtils::FlushPressedKeys(worldContext);
}

void UDrsInputBlueprintFunctionLibrary::SetCustomInputKey(APlayerController* controller, FName customKeyID, float value, ECustomInputKeyScope scope)
{
	if (controller)
	{
		if (value != 0.0f)
		{
			GetActiveCustomInputs(scope).Add(customKeyID);
			controller->InputKey(FInputKeyEventArgs::CreateSimulated(FKey(customKeyID), IE_Pressed, value));
		}
		else
		{
			GetActiveCustomInputs(scope).Remove(customKeyID);
			controller->InputKey(FInputKeyEventArgs::CreateSimulated(FKey(customKeyID), IE_Released, 0.0f));
		}
	}
}

void UDrsInputBlueprintFunctionLibrary::PressCustomInputKey(APlayerController* controller, FName customKeyID, ECustomInputKeyScope scope)
{
	if (controller)
	{
		GetActiveCustomInputs(scope).Add(customKeyID);
		controller->InputKey(FInputKeyEventArgs::CreateSimulated(FKey(customKeyID), IE_Pressed, 1.0f));
	}
}

void UDrsInputBlueprintFunctionLibrary::ReleaseCustomInputKey(APlayerController* controller, FName customKeyID, ECustomInputKeyScope scope)
{
	if (controller)
	{
		GetActiveCustomInputs(scope).Remove(customKeyID);
		controller->InputKey(FInputKeyEventArgs::CreateSimulated(FKey(customKeyID), IE_Released, 0.0f));
	}
}

void UDrsInputBlueprintFunctionLibrary::ResetCustomInputKeys(APlayerController* controller, ECustomInputKeyScope scope)
{
	TSet<FName>& activeKeys = GetActiveCustomInputs(scope);
	if (controller)
	{
		for (FName key : activeKeys)
		{
			controller->InputKey(FInputKeyEventArgs::CreateSimulated(FKey(key), IE_Released, 0.0f));
		}
	}
	activeKeys.Reset();
}
