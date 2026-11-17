// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class UInputAction;
struct FInputContextDefinition;
struct FInputContextDefinitionTwoHands;

namespace DrsInputUtils
{
	const FName DefaultInputReason("DrsDefault");
	
	DRSINPUT_API UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem(const UObject* worldContext);

	DRSINPUT_API void AddInputContext(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContext(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason = DefaultInputReason);

	DRSINPUT_API void AddInputContext(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EControllerHand hand, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContext(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EControllerHand hand, FName reason = DefaultInputReason);
	
	DRSINPUT_API void AddInputContext(const UObject* worldContext, const UInputMappingContext* context, int32 priority, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContext(const UObject* worldContext, const UInputMappingContext* context, FName reason = DefaultInputReason);
	
	DRSINPUT_API void AddInputContextToSubsystem(UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, int32 priority, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContextFromSubsystem(UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, FName reason = DefaultInputReason);

	DRSINPUT_API bool HasMappingForAction(const UInputMappingContext* context, const UInputAction* action);
	DRSINPUT_API bool TryGetHandForInputAction(const UInputAction* action, EControllerHand& outHand);
	DRSINPUT_API EControllerHand GetHandForInputAction(const UInputAction* action, const UInputMappingContext* leftHandContext, const UInputMappingContext* rightHandContext);

	DRSINPUT_API void ClearBindingsForContext(UEnhancedInputComponent* inputComponent, const UObject* owner, const UInputMappingContext* context);
	DRSINPUT_API void ClearBindingsForHand(UEnhancedInputComponent* inputComponent, UObject* owner, EControllerHand hand);

	DRSINPUT_API void ReleaseDanglingInputs(const UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context);
	DRSINPUT_API void ReleaseOverriddenInputs(const UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, int32 priority);

	DRSINPUT_API void FlushPressedKeys(const UObject* worldContext);
}

FORCEINLINE void DrsInputUtils::AddInputContext(const UObject* worldContext, const UInputMappingContext* context, int32 priority, FName reason)
{
	AddInputContextToSubsystem(GetEnhancedInputSubsystem(worldContext), context, priority, reason);
}

FORCEINLINE void DrsInputUtils::RemoveInputContext(const UObject* worldContext, const UInputMappingContext* context, FName reason)
{
	RemoveInputContextFromSubsystem(GetEnhancedInputSubsystem(worldContext), context, reason);
}
