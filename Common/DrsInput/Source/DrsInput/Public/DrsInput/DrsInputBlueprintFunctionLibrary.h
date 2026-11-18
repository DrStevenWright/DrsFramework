// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsBase/DrsHand.h"

#include "InputCoreTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DrsInputBlueprintFunctionLibrary.generated.h"

class UInputMappingContext;

/**
 * Optional scope for a custom input key. The scope only groups the keys that were pressed, so that ResetCustomInputKeys
 * can release one group (for example one hand's) without touching the others. Nothing releases a scope automatically:
 * whatever changes the hand's control mode must call ResetCustomInputKeys itself.
 */
UENUM(BlueprintType)
enum class EDrsCustomInputKeyScope : uint8
{
	Global,
	LeftHand,
	RightHand,
	NumScopes UMETA(Hidden)
};

UCLASS()
class DRSINPUT_API UDrsInputBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	* Returns a handle to DrsInput's developer settings object.
	*/
	UFUNCTION(BlueprintPure, Category = "DRS|Input")
	static const class UDrsInputSettings* GetDrsInputSettings();
	
	/**
	 * Adds an InputMappingContext to the enhanced input system at a specified priority.
	 * @param context	InputMappingContext to add.
	 * @param priority	Priority at which this context should be added. Higher priority contexts will override inputs
	 *                  from lower priority contexts.
	 * @param reason	Allows us to specify the reason why this context needs to be being added. An input context added
	 *                  in this way will only be removed once all specified reasons are withdrawn. Specify 'None' for
	 *                  default binary add/remove behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void AddInputContext(const UObject* worldContext, const UInputMappingContext* context, int32 priority, FName reason);
	
	/**
	 * Removes an InputMappingContext from the enhanced input system
	 * @param context	InputMappingContext to remove.
	 * @param reason	Allows us to specify the reason why this context needs to be removed (in other words, withdraw
	 *                  the reason the context was added in the first place). Specify 'None' for default binary add/remove
	 *                  behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void RemoveInputContext(const UObject* worldContext, const UInputMappingContext* context, FName reason);
	
	/**
	 * Adds an InputMappingContext to the enhanced input system at a pre-defined priority.
	 * @param contextDefinition	InputMappingContext to add.
	 * @param reason	Allows us to specify the reason why this context needs to be being added. An input context added
	 *                  in this way will only be removed once all specified reasons are withdrawn. Specify 'None' for
	 *                  default binary add/remove behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void AddInputContextDefinition(const UObject* worldContext, const FDrsInputContextDefinition& contextDefinition, FName reason);

	/**
	 * Removes an InputMappingContext from the enhanced input system
	 * @param contextDefinition	InputMappingContext to remove.
	 * @param reason	Allows us to specify the reason why this context needs to be removed (in other words, withdraw
	 *                  the reason the context was added in the first place). Specify 'None' for default binary add/remove
	 *                  behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void RemoveInputContextDefinition(const UObject* worldContext, const FDrsInputContextDefinition& contextDefinition, FName reason);

	/**
	 * Adds hand-specific InputMappingContext(s) to the enhanced input system at a pre-defined priority.
	 * @param contextDefinition	(Two-handed) InputMappingContext to add.
	 * @param hands	Specifies whether to add the left-hand context, the right-hand context or both.
	 * @param reason	Allows us to specify the reason why this context needs to be being added. An input context added
	 *                  in this way will only be removed once all specified reasons are withdrawn. Specify 'None' for
	 *                  default binary add/remove behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void AddInputContextDefinitionForHands(const UObject* worldContext, const FDrsInputContextDefinitionTwoHands& contextDefinition, EDrsHandMask hands, FName reason);

	/**
	 * Removes hand-specific InputMappingContext(s) from the enhanced input system.
	 * @param contextDefinition	(Two-handed) InputMappingContext to remove.
	 * @param hands	Specifies whether to remove the left-hand context, the right-hand context or both.
	* @param reason		Allows us to specify the reason why this context needs to be removed (in other words, withdraw
	 *                  the reason the context was added in the first place). Specify 'None' for default binary add/remove
	 *                  behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void RemoveInputContextDefinitionForHands(const UObject* worldContext, const FDrsInputContextDefinitionTwoHands& contextDefinition, EDrsHandMask hands, FName reason);

	/**
	 * Flushes all currently pressed keys in the input system.
	 */
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void FlushPressedKeys(const UObject* worldContext);

	//--------------------------------------------------------------------------------------------------------
	// Custom input keys
	//--------------------------------------------------------------------------------------------------------

	// Sets input value on a custom input key. (Pressed/released states are determined automatically based on supplied value.)
	// (The input can optionally be scoped to a specific hand.)
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void SetCustomInputKey(APlayerController* controller, FName customKeyID, float value = 1.0f, EDrsCustomInputKeyScope scope = EDrsCustomInputKeyScope::Global);

	// Sets pressed state on a custom input key.
	// (The input can optionally be scoped to a specific hand.)
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void PressCustomInputKey(APlayerController* controller, FName customKeyID, EDrsCustomInputKeyScope scope = EDrsCustomInputKeyScope::Global);

	// Removes pressed state on a custom input key.
	// (The input can optionally be scoped to a specific hand.)
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void ReleaseCustomInputKey(APlayerController* controller, FName customKeyID, EDrsCustomInputKeyScope scope = EDrsCustomInputKeyScope::Global);

	// Clears pressed state on all custom input keys within a specific scope.
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void ResetCustomInputKeys(APlayerController* controller, EDrsCustomInputKeyScope scope = EDrsCustomInputKeyScope::Global);
};
