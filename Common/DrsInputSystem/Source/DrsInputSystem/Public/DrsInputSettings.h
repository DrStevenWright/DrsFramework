// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
//#include "DrsInputMappingContext.h"
#include "Engine/DeveloperSettings.h"
#include "DrsInputSettings.generated.h"

// SWTODO - Move into GameSupport.
/** Container for global interaction system settings. */
UCLASS(Config = Game, DefaultConfig)
class DRSINPUTSYSTEM_API UDrsInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	
	static const UDrsInputSettings& Get() { return *GetDefault<UDrsInputSettings>(); }
	
	/*
	// Controls that are always active.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinition GeneralControls;

	// Debug controls that are always active.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinition DebugControls;
	
	// Controls that are specific to VR mode.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input", DisplayName = "VR Mode Controls")
	FInputContextDefinition VrModeControls;

	// Controls that are specific to non-VR mode.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input", DisplayName = "Non-VR Mode Controls")
	FInputContextDefinition NonVrModeControls;

	// Controls that are specific to spectator mode.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinition SpectatorModeControls;
	
	// Per-device motion controller mappings for cross-platform support. Update this whenever a new device is added.
	// NOTE: This context should have maximum priority to prevent its inputs being overridden.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinition MotionControls;

	// Non-VR emulation of certain motion controller inputs.
	// NOTE: This context should have maximum priority to prevent its inputs being overridden.
	// NOTE: Actions mapped within this context should be configured NOT to consume input.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input", DisplayName = "Non-VR Motion Controls")
	FInputContextDefinition NonVrMotionControls;

	// Controls for driving hand animation and gestures.
	// NOTE: Typically this should have a very hgh priority so that gestures can always be performed.
	// NOTE: Actions mapped within this context should therefore be configured NOT to consume input.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinition GestureControls;
	
	// Controls for triggering teleports / rotation.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinition TeleportControls;
	
	// Controls that are active while any UPO is selected.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input", DisplayName = "UPO Controls")
	FInputContextDefinition UpoControls;

	// Controls for operating the laser pointers.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinitionTwoHands LaserPointerControls;
	
	// Controls that are active while hovering some interactable UI.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input", DisplayName = "UI Interaction Controls")
	FInputContextDefinitionTwoHands UIInteractionControls;

	// Controls that are active while hovering some scrollable UI.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input", DisplayName = "UI Scroll Controls")
	FInputContextDefinitionTwoHands UIScrollControls;

	// Controls that are specific to placeable objects (e.g. UPOs).
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	FInputContextDefinitionTwoHands PlacementControls;
	*/
	// Input threshold beyond which a thumbstick is considered to be 'deflected' in some direction. Used in corresponding
	// deflection input triggers.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	float ThumbstickDeflectionThreshold = 0.6f;
};
