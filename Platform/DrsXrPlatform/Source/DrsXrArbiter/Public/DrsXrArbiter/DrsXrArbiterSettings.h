// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"
#include "Engine/DeveloperSettings.h"

#include "DrsXrArbiterSettings.generated.h"

/**
 * Project settings for the XR arbiter's two independent choices: which OpenXR runtime it selects on Windows, and which
 * vendor XR plugin (such as MetaXR or PicoOpenXR) loads. Restart to apply changes.
 */
UCLASS(Config = DrsXrPlatform, DefaultConfig, DisplayName = "XR Arbiter")
class DRSXRARBITER_API UDrsXrArbiterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDrsXrArbiterSettings()
	{
		// Project Settings > DRS > XR Arbiter.
		CategoryName = TEXT("DRS");
	}

	/** The OpenXR runtimes DRS knows how to find. */
	UPROPERTY(Config, EditAnywhere, Category = "OpenXR Runtime", meta = (TitleProperty = "Name"))
	TArray<FDrsXrRuntime> Runtimes;

	/**
	 * The runtimes to consider, by name, most preferred first. The first one that is installed and running is selected.
	 * With none running, the system's active OpenXR runtime is used.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "OpenXR Runtime")
	TArray<FName> RuntimePriority;

	/**
	 * Forces the XR plugin for this project, whatever the hardware. Use it only where the hardware is known and fixed,
	 * not for projects that run on devices from several vendors. None leaves the choice to the hardware.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Vendor Plugin", DisplayName = "Project Override", meta = (GetOptions = "GetXrPluginOptions"))
	FName XrArbiterProjectOverride;

	/** The vendor XR plugins, and the words that identify their hardware. The first vendor that matches the hardware is chosen. */
	UPROPERTY(Config, EditAnywhere, Category = "Vendor Plugin", meta = (TitleProperty = "Id"))
	TArray<FDrsXrVendor> Vendors;

	/** None, then each vendor's ID: the options for the project override and the editor override (UDrsXrPlatformEditorSettings). */
	UFUNCTION()
	static TArray<FString> GetXrPluginOptions();
};
