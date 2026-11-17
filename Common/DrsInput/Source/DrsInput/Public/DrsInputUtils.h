// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsHand.h"

class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class UInputAction;
struct FDrsInputContextDefinition;
struct FDrsInputContextDefinitionTwoHands;

namespace Drs::Input
{
	// Reason used when a C++ caller doesn't give one. It is deliberately not NAME_None, which is the default reason that
	// Blueprint callers use for add/remove behaviour, so that Blueprint logic never withdraws a reason that C++ code added.
	// C++ callers should normally pass a reason of their own that says why the context is needed.
	const FName DefaultInputReason("DrsDefault");
	
	DRSINPUT_API UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem(const UObject* worldContext);

	DRSINPUT_API void AddInputContext(const UObject* worldContext, const FDrsInputContextDefinition& contextDefinition, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContext(const UObject* worldContext, const FDrsInputContextDefinition& contextDefinition, FName reason = DefaultInputReason);

	DRSINPUT_API void AddInputContext(const UObject* worldContext, const FDrsInputContextDefinitionTwoHands& contextDefinition, EDrsHandMask hands, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContext(const UObject* worldContext, const FDrsInputContextDefinitionTwoHands& contextDefinition, EDrsHandMask hands, FName reason = DefaultInputReason);
	
	DRSINPUT_API void AddInputContext(const UObject* worldContext, const UInputMappingContext* context, int32 priority, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContext(const UObject* worldContext, const UInputMappingContext* context, FName reason = DefaultInputReason);
	
	DRSINPUT_API void AddInputContextToSubsystem(UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, int32 priority, FName reason = DefaultInputReason);
	DRSINPUT_API void RemoveInputContextFromSubsystem(UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, FName reason = DefaultInputReason);

	DRSINPUT_API bool HasMappingForAction(const UInputMappingContext* context, const UInputAction* action);
	
	/** Gets the hand that an action is tagged with. Unset if the action is not tagged with exactly one hand. */
	DRSINPUT_API TOptional<EDrsHand> GetHandForInputAction(const UInputAction* action);

	/** Gets the hand that an action is tagged with or, failing that, the hand whose context maps it. Unset if neither identifies one hand. */
	DRSINPUT_API TOptional<EDrsHand> GetHandForInputAction(const UInputAction* action, const UInputMappingContext* leftHandContext, const UInputMappingContext* rightHandContext);

	DRSINPUT_API void ClearBindingsForContext(UEnhancedInputComponent* inputComponent, const UObject* owner, const UInputMappingContext* context);
	
	/**
	 * Clears the owner's bindings for actions tagged with exactly the given hand. Specifying Both clears every binding the
	 * owner has on the component, including those for actions that are not hand-specific. None clears nothing.
	 */
	DRSINPUT_API void ClearBindingsForHands(UEnhancedInputComponent* inputComponent, UObject* owner, EDrsHandMask hands);

	DRSINPUT_API void ReleaseDanglingInputs(const UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context);
	DRSINPUT_API void ReleaseOverriddenInputs(const UEnhancedInputLocalPlayerSubsystem* enhancedInput, const UInputMappingContext* context, int32 priority);

	DRSINPUT_API void FlushPressedKeys(const UObject* worldContext);
}

FORCEINLINE void Drs::Input::AddInputContext(const UObject* worldContext, const UInputMappingContext* context, int32 priority, FName reason)
{
	AddInputContextToSubsystem(GetEnhancedInputSubsystem(worldContext), context, priority, reason);
}

FORCEINLINE void Drs::Input::RemoveInputContext(const UObject* worldContext, const UInputMappingContext* context, FName reason)
{
	RemoveInputContextFromSubsystem(GetEnhancedInputSubsystem(worldContext), context, reason);
}
