// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

#include "Engine/DeveloperSettings.h"
#include "DrsXrPlatformEditorSettings.generated.h"

/**
 * Per-user editor setting: which vendor XR plugin the editor loads. The arbiter reads it from config at startup, so it
 * takes effect after an editor restart.
 */
UCLASS(Config = EditorSettings, DisplayName = "DRS XR Plugin Selection")
class DRSXRPLATFORMEDITOR_API UDrsXrPlatformEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("DRS"); }

	/**
	 * The vendor XR plugin to load in the editor, so that its project settings can be edited and PIE runs with it.
	 * None leaves the choice to the project override and the arbitration rules.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "XR", meta = (ConfigRestartRequired = true))
	EDrsXrPluginType EditorXrConfiguration = EDrsXrPluginType::None;
};
