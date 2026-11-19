// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"

#include "DrsXrPlatformSettings.generated.h"

/** Which inputs the project supports: physical controllers, tracked hands, or both. */
UENUM()
enum class EDrsXrHandTrackingSupport : uint8
{
	/** Physical controllers only. Hand tracking is off, and Quest packages don't declare it. */
	ControllersOnly,
	/** Physical controllers and tracked hands. */
	ControllersAndHands,
	/** Tracked hands only. Quest packages require hand tracking, so the Store won't offer them to devices without it. */
	HandsOnly,
};

/** How often hands are tracked. High is more responsive but costs performance. */
UENUM()
enum class EDrsXrHandTrackingFrequency : uint8
{
	Low,
	High,
};

// Config = Engine, not the plugin's own file, because DrsXrPlatform_UPL.xml reads these settings, and UPL can only read
// the fixed config hierarchies (Engine, Game, ...).

/**
 * Project settings for the XR platform on every device, and for how Android packages declare its features. Packaging
 * settings apply to the next Android package.
 */
UCLASS(Config = Engine, DefaultConfig, DisplayName = "XR Platform")
class DRSXRPLATFORM_API UDrsXrPlatformSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDrsXrPlatformSettings()
	{
		// Project Settings > DRS > XR Platform.
		CategoryName = TEXT("DRS");
	}

	/** Whether the project supports tracked hands, physical controllers, or both. */
	UPROPERTY(Config, EditAnywhere, Category = "Hand Tracking")
	EDrsXrHandTrackingSupport HandTrackingSupport = EDrsXrHandTrackingSupport::ControllersAndHands;

	/** Tracking frequency for hand-tracking. Higher responsiveness comes at the cost of performance. */
	UPROPERTY(Config, EditAnywhere, Category = "Hand Tracking")
	EDrsXrHandTrackingFrequency HandTrackingFrequency = EDrsXrHandTrackingFrequency::High;
};
