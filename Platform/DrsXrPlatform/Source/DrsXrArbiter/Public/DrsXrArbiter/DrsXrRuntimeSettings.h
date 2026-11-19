// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"
#include "Engine/DeveloperSettings.h"

#include "DrsXrRuntimeSettings.generated.h"

/** Project settings for which OpenXR runtime the XR arbiter selects on Windows. Restart to apply changes. */
UCLASS(Config = DrsXrPlatform, DefaultConfig, DisplayName = "XR Runtimes")
class DRSXRARBITER_API UDrsXrRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDrsXrRuntimeSettings()
	{
		// Project Settings > DRS > XR Runtimes.
		CategoryName = TEXT("DRS");
	}

	/** The OpenXR runtimes DRS knows how to find. */
	UPROPERTY(Config, EditAnywhere, Category = NoCategory, meta = (TitleProperty = "Name"))
	TArray<FDrsXrRuntime> Runtimes;

	/**
	 * The runtimes to consider, by name, most preferred first. A running runtime is selected over one that is only
	 * installed; otherwise the first installed one is. With none installed, the system's active OpenXR runtime is used.
	 */
	UPROPERTY(Config, EditAnywhere, Category = NoCategory)
	TArray<FName> RuntimePriority;
};
