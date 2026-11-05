// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSXRPLATFORM_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrPlatform, Log, All);

class FDrsXrPlatformModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
