// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsHand.h"
#include "InputMappingContext.h"
#include "DrsInputMappingContext.generated.h"

/** Custom class for input mapping contexts used by Drs. */
UCLASS(BlueprintType)
class DRSINPUT_API UDrsInputMappingContext : public UInputMappingContext
{
	GENERATED_BODY()

public:

	UDrsInputMappingContext()
	{
		ContextDescription = FText::AsCultureInvariant(ContextDescription);
	}
	
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	
	const UInputAction* FindActionByName(FName actionName) const;
	const UInputAction* FindActionByID(FName actionID) const;
	TArray<FKey> FindKeysForActionID(FName actionID) const;

private:
	
	UPROPERTY(Transient)
	mutable TSet<const class UDrsInputAction*> DrsInputActions;

#if WITH_EDITOR
	FDelegateHandle PIEBeginHandle;
#endif
};

/** Definition structure for grouping input contexts alongside their priority. */
USTRUCT(BlueprintType)
struct DRSINPUT_API FInputContextDefinition
{
	GENERATED_BODY()

	bool IsValid() const { return Context != nullptr; }
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Drs|Input")
	const UDrsInputMappingContext* Context = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Drs|Input")
	int32 Priority = 0;
};

/** Definition structure for grouping input contexts alongside their priority. */
USTRUCT(BlueprintType)
struct DRSINPUT_API FInputContextDefinitionTwoHands
{
	GENERATED_BODY()

	bool IsValid(const EDrsHand hand) const
	{
		return GetContextForHand(hand) != nullptr;
	}

	const UDrsInputMappingContext* GetContextForHand(const EDrsHand hand) const
	{
		return (hand == EDrsHand::Left) ? LeftHandContext : RightHandContext;
	}

	const UInputAction* FindActionByName(const FName actionName) const
	{
		if (const UInputAction* action = LeftHandContext ? LeftHandContext->FindActionByName(actionName) : nullptr)
		{
			return action;
		}
		return RightHandContext ? RightHandContext->FindActionByName(actionName) : nullptr;
	}

	const UInputAction* FindActionByID(const FName actionID) const
	{
		if (const UInputAction* action = LeftHandContext ? LeftHandContext->FindActionByID(actionID) : nullptr)
		{
			return action;
		}
		return RightHandContext ? RightHandContext->FindActionByID(actionID) : nullptr;
	}
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Drs|Input")
	const UDrsInputMappingContext* LeftHandContext = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Drs|Input")
	const UDrsInputMappingContext* RightHandContext = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Drs|Input")
	int32 Priority = 0;
};
