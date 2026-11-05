// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSMATHUTILITIES_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsMathUtilities, Log, All);

class FDrsMathUtilitiesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
