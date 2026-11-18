// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"
#include "Engine/DeveloperSettings.h"

#include "DrsXrArbiterSettings.generated.h"

/** Project settings for how DRS chooses which vendor XR plugin (such as Meta or Pico) the project loads. */
UCLASS(Config = Game, DefaultConfig, DisplayName = "XR Plugin Arbiter")
class DRSXRARBITER_API UDrsXrArbiterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDrsXrArbiterSettings()
	{
		// Project Settings > DRS > XR Plugin Arbiter.
		CategoryName = TEXT("DRS");
	}

	/**
	 * Forces the XR plugin type for this project, overriding the rules. Use it only where the hardware is known and fixed,
	 * not for projects that run on devices from several vendors.
	 */
	UPROPERTY(Config, EditAnywhere, Category = NoCategory, DisplayName = "XR Plugin Arbiter - Project Override")
	EDrsXrPluginType XrArbiterProjectOverride = EDrsXrPluginType::None;

	/** Rules that pick the plugin type on standalone (Android) devices. */
	UPROPERTY(Config, EditAnywhere, Category = NoCategory)
	TArray<FDrsArbitrationRule> MobileDeviceArbitrationRules;

	/** Rules that pick the plugin type for tethered (Windows) devices. */
	UPROPERTY(Config, EditAnywhere, Category = NoCategory)
	TArray<FDrsArbitrationRule> TetheredDeviceArbitrationRules;
};
