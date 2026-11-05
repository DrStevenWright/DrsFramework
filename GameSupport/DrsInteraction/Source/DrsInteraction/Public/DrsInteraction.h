// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DRSINTERACTION_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsInteraction, Log, All);

class FDrsInteractionModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
