// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsXrArbiterTypes.h"
#include "Engine/DeveloperSettings.h"
#include "DrsXrArbiterSettings.generated.h"

/**
 * Project settings for the XR plugin arbiter. The arbiter reads this section from config directly, because it decides
 * before this object is loaded; the class is here so the settings can be edited in the editor.
 */
UCLASS(Config = Game, DefaultConfig, DisplayName = "Drs XR Plugin Arbiter")
class DRSXRARBITER_API UDrsXrArbiterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("DrsFramework"); }

	/**
	 * Forces the XR plugin type for this project, overriding the rules. Use it only where the hardware is known and fixed,
	 * not for projects that run on devices from several vendors.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Plugin Arbiter")
	EDrsXrPluginType ProjectArbitrationOverride = EDrsXrPluginType::None;

	/** Rules that pick the plugin type on standalone (Android) devices. */
	UPROPERTY(Config, EditAnywhere, Category = "Plugin Arbiter")
	TArray<FDrsArbitrationRule> MobileDeviceArbitrationRules;

	/** Rules that pick the plugin type for tethered (Windows) devices. */
	UPROPERTY(Config, EditAnywhere, Category = "Plugin Arbiter")
	TArray<FDrsArbitrationRule> TetheredDeviceArbitrationRules;
};
