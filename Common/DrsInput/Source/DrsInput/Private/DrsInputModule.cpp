// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMotionControllerKeys.h"
#include "Modules/ModuleManager.h"

class FDrsInputModule : public FDefaultGameModuleImpl
{
public:

	virtual void StartupModule() override
	{
		Drs::MotionControllerKeys::Register();
	}
};

IMPLEMENT_GAME_MODULE(FDrsInputModule, DrsInput);
