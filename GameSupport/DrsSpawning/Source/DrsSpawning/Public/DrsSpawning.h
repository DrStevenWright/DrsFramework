// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSSPAWNING_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsSpawning, Log, All);

class FDrsSpawningModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
