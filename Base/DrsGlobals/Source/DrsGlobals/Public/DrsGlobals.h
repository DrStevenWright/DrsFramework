// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSGLOBALS_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsGlobals, Log, All);

class FDrsGlobalsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
