// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"
#include "Engine/DeveloperSettings.h"

#include "DrsXrArbiterSettings.generated.h"

/** Project settings for how the XR arbiter chooses which vendor XR plugin (such as Meta or Pico) loads. Restart to apply changes. */
UCLASS(Config = DrsXrPlatform, DefaultConfig, DisplayName = "XR Vendor Plugins")
class DRSXRARBITER_API UDrsXrArbiterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDrsXrArbiterSettings()
	{
		// Project Settings > DRS > XR Vendor Plugins.
		CategoryName = TEXT("DRS");
	}

	/**
	 * Forces the XR plugin for this project, whatever the hardware. Use it only where the hardware is known and fixed,
	 * not for projects that run on devices from several vendors. None leaves the choice to the hardware.
	 */
	UPROPERTY(Config, EditAnywhere, Category = NoCategory, DisplayName = "Project Override", meta = (GetOptions = "GetXrPluginOptions"))
	FName XrArbiterProjectOverride;

	/** The vendor XR plugins, and the words that identify their hardware. The first vendor that matches the hardware is chosen. */
	UPROPERTY(Config, EditAnywhere, Category = NoCategory, meta = (TitleProperty = "Id"))
	TArray<FDrsXrVendor> Vendors;

	/** None, then each vendor's ID: the options for the project override and the editor override (UDrsXrPlatformEditorSettings). */
	UFUNCTION()
	static TArray<FString> GetXrPluginOptions();
};
