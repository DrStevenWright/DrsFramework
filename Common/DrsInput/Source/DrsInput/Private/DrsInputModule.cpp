// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInputLog.h"
#include "DrsMotionControllerKeys.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsInput)

class FDrsInputModule : public IModuleInterface
{
public:

	virtual void StartupModule() override
	{
		Drs::MotionControllerKeys::Register();
	}

	virtual void ShutdownModule() override
	{
		Drs::MotionControllerKeys::Unregister();
	}
};

IMPLEMENT_MODULE(FDrsInputModule, DrsInput)
