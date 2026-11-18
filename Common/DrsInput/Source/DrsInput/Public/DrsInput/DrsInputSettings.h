// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"

#include "DrsInputSettings.generated.h"

/** Project settings for DRS input systems. */
UCLASS(Config = Game, DefaultConfig, DisplayName = "Input")
class DRSINPUT_API UDrsInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UDrsInputSettings()
	{
		// Project Settings > DRS > Input.
		CategoryName = TEXT("DRS");
	}

	static const UDrsInputSettings& Get() { return *GetDefault<UDrsInputSettings>(); }

	// Input threshold beyond which a thumbstick is considered to be 'deflected' in some direction. Used in corresponding
	// deflection input triggers.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = NoCategory)
	float ThumbstickDeflectionThreshold = 0.6f;
};
