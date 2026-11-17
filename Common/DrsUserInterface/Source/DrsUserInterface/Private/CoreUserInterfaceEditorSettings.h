// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CoreUserInterfaceEditorSettings.generated.h"

/**
 * Editor-only settings relating to Core's UI interaction implementation.
 */
UCLASS(config=EditorSettings, MinimalAPI, DisplayName = "Core UI Interaction Settings")
class UCoreUserInterfaceEditorSettings : public UDeveloperSettings
{
public:
	
	GENERATED_BODY()

	static const UCoreUserInterfaceEditorSettings& Get() { return *GetDefault<UCoreUserInterfaceEditorSettings>(); }
	
	// UDeveloperSettings overrides
	virtual FName GetContainerName() const override { return FName("Editor"); }
	virtual FName GetCategoryName() const override { return FName("RW1"); }
	virtual FName GetSectionName() const override { return FName("Core UI Interaction"); }
	
	// Inertial scrolling of UI elements employs right mouse button events which, on some occasions, can cause editor
	// context menus to be triggered. This option allows you to disable inertial scroll while using the editor.
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Core UI Interaction")
	bool bDisableInertialScrollUsingPointer = false;
};
