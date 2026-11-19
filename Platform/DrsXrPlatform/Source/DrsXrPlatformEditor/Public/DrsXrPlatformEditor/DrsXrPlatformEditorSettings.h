// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"
#include "Engine/DeveloperSettings.h"

#include "DrsXrPlatformEditorSettings.generated.h"

/** Local user's editor preferences pertaining to DRS's platform-level XR support. */
UCLASS(Config = EditorSettings, DisplayName = "XR Platform")
class DRSXRPLATFORMEDITOR_API UDrsXrPlatformEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDrsXrPlatformEditorSettings()
	{
		// Editor Preferences > DRS > XR Platform.
		CategoryName = TEXT("DRS");
	}

	/**
	 * If the XR arbiter selects between several installed vendor XR plugins, this option overrides which plugin loads
	 * in the editor. For example, use this if you need to edit the plugin settings without first connecting hardware of
	 * the corresponding type. None leaves the choice to the project override and the hardware.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "XR Vendor Plugin", DisplayName = "Editor Override", meta = (ConfigRestartRequired = true, GetOptions = "DrsXrArbiter.DrsXrArbiterSettings.GetXrPluginOptions"))
	FName XrArbiterEditorOverride;
};
