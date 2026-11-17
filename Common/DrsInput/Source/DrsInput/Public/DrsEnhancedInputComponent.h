// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsInputMappingContext.h"
#include "DrsInputAction.h"
#include "EnhancedInputComponent.h"
#include "DrsEnhancedInputComponent.generated.h"

/** Custom version of EnhancedInputComponent to support Drs APIs. */
UCLASS(transient, config=Input, hidecategories=(Activation, "Components|Activation"))
class DRSINPUT_API UDrsEnhancedInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	
	UDrsEnhancedInputComponent() {}

	// Engine callbacks
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	
	// --------------------------------------------------------------
	// Input interruption handling
	// --------------------------------------------------------------
	static void ReleaseDanglingInputs(const TArray<const FInputActionInstance*>& danglingActionInstances);
	static void ReleaseOverriddenInputs(const TArray<const FInputActionInstance*>& overriddenActionInstances);
	
	// --------------------------------------------------------------
	// Action binding macros - Adapted from UEnhancedInputComponent
	//---------------------------------------------------------------
	// TODO: To be consistent with the parent class, these custom binding functions should return a FEnhancedInputActionEventBinding
	// reference. But what should we return in the cases where the binding failed...?
	
	#define DEFINE_BIND_NAMED_ACTION(HANDLER_SIG)																																												\
		template<class UserClass, typename... VarTypes>																																											\
		void BindNamedAction(const UDrsInputMappingContext* context, FName actionName, ETriggerEvent triggerEvent, UserClass* object, typename HANDLER_SIG::template TMethodPtr<UserClass, VarTypes...> func, VarTypes... vars)\
		{																												\
			if (const UInputAction* inputAction = context ? context->FindActionByName(actionName) : nullptr)			\
			{																											\
				BindAction(inputAction, triggerEvent, object, func, vars...);											\
			}																											\
		}

	#define DEFINE_BIND_TAGGED_ACTION(HANDLER_SIG)																																										\
		template<class UserClass, typename... VarTypes>																																									\
		void BindActionID(const UDrsInputMappingContext* context, FName ID, ETriggerEvent triggerEvent, UserClass* object, typename HANDLER_SIG::template TMethodPtr<UserClass, VarTypes...> func, VarTypes... vars)	\
		{																												\
			if (const UInputAction* inputAction = context ? context->FindActionByID(ID) : nullptr)						\
			{																											\
				BindAction(inputAction, triggerEvent, object, func, vars...);											\
			}																											\
		}

	DEFINE_BIND_NAMED_ACTION(FEnhancedInputActionHandlerSignature);
	DEFINE_BIND_NAMED_ACTION(FEnhancedInputActionHandlerValueSignature);
	DEFINE_BIND_NAMED_ACTION(FEnhancedInputActionHandlerInstanceSignature);

	DEFINE_BIND_TAGGED_ACTION(FEnhancedInputActionHandlerSignature);
	DEFINE_BIND_TAGGED_ACTION(FEnhancedInputActionHandlerValueSignature);
	DEFINE_BIND_TAGGED_ACTION(FEnhancedInputActionHandlerInstanceSignature);
	
private:
	
	void ReleaseInterruptedInputs(const TArray<const FInputActionInstance*>& interruptedActionInstances) const;
	
	using FOnNotifyInputInterrupt = TMulticastDelegate<void(const TArray<const FInputActionInstance*>& interruptedActionInstances)>;
	static FOnNotifyInputInterrupt OnNotifyDanglingInput;
	static FOnNotifyInputInterrupt OnNotifyOverriddenInput;
	};
