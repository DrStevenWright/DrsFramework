// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSBASEUTILITIES_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsBaseUtilities, Log, All);

class FDrsBaseUtilitiesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
