// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInput/DrsInputLog.h"
#include "DrsInput/MotionController/DrsMotionControllerKeys.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsInput)

class FDrsInputModule : public IModuleInterface
{
public:

	virtual void StartupModule() override
	{
		Drs::Input::RegisterMotionControllerKeys();
	}

	virtual void ShutdownModule() override
	{
		Drs::Input::UnregisterMotionControllerKeys();
	}
};

IMPLEMENT_MODULE(FDrsInputModule, DrsInput)
