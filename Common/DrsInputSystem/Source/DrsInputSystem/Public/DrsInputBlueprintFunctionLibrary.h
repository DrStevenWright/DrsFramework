// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DrsInputBlueprintFunctionLibrary.generated.h"

/** 
 * Optional scope for a custom input key, allows us to associate an input with a specific hand so that active inputs can
 * be automatically cleaned up when that hand changes control mode, for example. Globally-scoped custom inputs will not be
 * cleaned up except via an appropriate call to ResetCustomInputKeys.
 */
UENUM(BlueprintType)
enum class ECustomInputKeyScope : uint8
{
	Global,
	LeftHand,
	RightHand,
	NumScopes UMETA(Hidden)
};

UCLASS()
class DRSINPUTSYSTEM_API UDrsInputBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	* Returns a handle to the CoreInputSystem's developer settings object.
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
	static void AddInputContextDefinition(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason);

	/**
	 * Removes an InputMappingContext from the enhanced input system
	 * @param contextDefinition	InputMappingContext to remove.
	 * @param reason	Allows us to specify the reason why this context needs to be removed (in other words, withdraw
	 *                  the reason the context was added in the first place). Specify 'None' for default binary add/remove
	 *                  behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void RemoveInputContextDefinition(const UObject* worldContext, const FInputContextDefinition& contextDefinition, FName reason);

	/**
	 * Adds a hand-specific InputMappingContext to the enhanced input system at a pre-defined priority.
	 * @param contextDefinition	(Two-handed) InputMappingContext to add.
	 * @param hand		Specifies whether to add the left- or right-hand input context.
	 * @param reason	Allows us to specify the reason why this context needs to be being added. An input context added
	 *                  in this way will only be removed once all specified reasons are withdrawn. Specify 'None' for
	 *                  default binary add/remove behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void AddInputContextDefinitionForHand(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EControllerHand hand, FName reason);

	/**
	 * Removes a hand-specific InputMappingContext to the enhanced input system at a pre-defined priority.
	 * @param contextDefinition	(Two-handed) InputMappingContext to remove.
	 * @param hand		Specifies whether to remove the left- or right-hand input context.
	* @param reason		Allows us to specify the reason why this context needs to be removed (in other words, withdraw
	 *                  the reason the context was added in the first place). Specify 'None' for default binary add/remove
	 *                  behaviour.
	*/
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void RemoveInputContextDefinitionForHand(const UObject* worldContext, const FInputContextDefinitionTwoHands& contextDefinition, EControllerHand hand, FName reason);

	/**
	 * Flushes all currently pressed keys in the input system.
	 */
	UFUNCTION(BlueprintCallable, Category = "DRS|Input", meta = (WorldContext = "worldContext"))
	static void FlushPressedKeys(const UObject* worldContext);

	//--------------------------------------------------------------------------------------------------------
	// Custom input keys
	//--------------------------------------------------------------------------------------------------------

	// Sets input value on a custom input key. (Pressed/released states are determined automatically based on supplied value.)
	// (The input can be optionally scoped to a specific hand, so that it will be automatically released when that hand
	// changes control mode.)
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void SetCustomInputKey(APlayerController* controller, FName customKeyID, float value = 1.0f, ECustomInputKeyScope scope = ECustomInputKeyScope::Global);

	// Sets pressed state on a custom input key.
	// (The input can be optionally scoped to a specific hand, so that it will be automatically released when that hand
	// changes control mode.)
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void PressCustomInputKey(APlayerController* controller, FName customKeyID, ECustomInputKeyScope scope = ECustomInputKeyScope::Global);

	// Removes pressed state on a custom input key.
	// (The input can be optionally scoped to a specific hand, so that it will be automatically released when that hand
	// changes control mode.)
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void ReleaseCustomInputKey(APlayerController* controller, FName customKeyID, ECustomInputKeyScope scope = ECustomInputKeyScope::Global);

	// Clears pressed state on all custom input keys within a specific scope.
	UFUNCTION(BlueprintCallable, Category = "DRS|Input")
	static void ResetCustomInputKeys(APlayerController* controller, ECustomInputKeyScope scope = ECustomInputKeyScope::Global);
};
