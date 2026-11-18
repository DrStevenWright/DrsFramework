// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

#include "Engine/DeveloperSettings.h"
#include "DrsXrPlatformEditorSettings.generated.h"

/** Your editor preferences for DRS's XR support. They apply to you only, not to the project. */
UCLASS(Config = EditorSettings, DisplayName = "XR Platform")
class DRSXRPLATFORMEDITOR_API UDrsXrPlatformEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDrsXrPlatformEditorSettings()
	{
		// Editor Preferences > DRS > XR Platform. The Editor container comes from Config = EditorSettings.
		CategoryName = TEXT("DRS");
	}

	/**
	 * If using the XR arbiter to select between different installed XR vendor plugins, then this option allows you to
	 * override what plugin is loaded in the editor, for example in case you need to edit the plugin settings.
	 * 'None' leaves the choice to the project override and the arbitration rules.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "XR Arbiter", meta = (ConfigRestartRequired = true))
	EDrsXrPluginType XrArbiterEditorOverride = EDrsXrPluginType::None;
};
