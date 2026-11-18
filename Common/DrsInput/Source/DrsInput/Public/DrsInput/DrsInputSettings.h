// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "DrsInputSettings.generated.h"

UCLASS(Config = Game, DefaultConfig)
class DRSINPUT_API UDrsInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UDrsInputSettings()
	{
		// Project Settings > Drs > Input.
		CategoryName = TEXT("DRS");
		SectionName = TEXT("Input");
	}

	static const UDrsInputSettings& Get() { return *GetDefault<UDrsInputSettings>(); }

	// Input threshold beyond which a thumbstick is considered to be 'deflected' in some direction. Used in corresponding
	// deflection input triggers.
	UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "DRS|Input")
	float ThumbstickDeflectionThreshold = 0.6f;
};
