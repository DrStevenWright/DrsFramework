// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CoreUserInterfaceSettings.generated.h"


/**
 * Container for global UI interaction settings.
 */
UCLASS(Config = Game, DefaultConfig, DisplayName = "Core UI Interaction Settings")
class DRSUSERINTERFACE_API UCoreUserInterfaceSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	
	static const UCoreUserInterfaceSettings& Get() { return *GetDefault<UCoreUserInterfaceSettings>(); }

	UFUNCTION(BlueprintCallable)
	static UCoreUserInterfaceSettings* GetCoreUserInterfaceSettings() { return GetMutableDefault<UCoreUserInterfaceSettings>(); }
	
	// UI input global scroll speed when in VR mode (in Slate Screen Units).
	UPROPERTY(Config, BlueprintReadWrite, BlueprintSetter = "SetUIInputScrollSpeedVR", EditDefaultsOnly, Category = "UI", DisplayName = "UI Input Scroll Speed VR")
	float UIInputScrollSpeedVR = 32.0f;
	
	// UI input global scroll speed when in Non-VR mode (in Slate Screen Units).
	UPROPERTY(Config, BlueprintReadWrite, BlueprintSetter = "SetUIInputScrollSpeedNonVR", EditDefaultsOnly, Category = "UI", DisplayName = "UI Input Scroll Speed Non-VR")
	float UIInputScrollSpeedNonVR = 85.0f;
	
	// Global multiplier on the allowed mouse deviation between pressing a releasing a button. Increasing this value makes
	// for more tolerant UI interaction, which may be needed for interaction using tracked-hands, for example. (Note, however,
	// that increasing this value also increases how far the mouse must be moved before drag-scrolling will start.)
	UPROPERTY(Config, BlueprintReadWrite, EditDefaultsOnly, Category = "UI")
	float MaxMouseDeviationMultiplier = 1.0f;

	// Set the UI input scroll speed for VR mode.
	UFUNCTION(BlueprintSetter, Category = "UI")
	void SetUIInputScrollSpeedVR(float newScrollSpeed);

	// Set the UI input scroll speed for Non-VR mode.
	UFUNCTION(BlueprintSetter, Category = "UI")
	void SetUIInputScrollSpeedNonVR(float newScrollSpeed);

	// Applies the Slate.GlobalScrollAmount cvar based on the current VR (UIInputScrollSpeedVR) or Non-VR (UIInputScrollSpeedNonVR) mode.
	void ApplyInputScrollSpeedSetting();
};
