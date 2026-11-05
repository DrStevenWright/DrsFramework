// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsLocomotion.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsLocomotion);

void FDrsLocomotionModule::StartupModule()
{
	DRS_LOG(LogDrsLocomotion, Verbose, TEXT("Module started"));
}

void FDrsLocomotionModule::ShutdownModule()
{
	DRS_LOG(LogDrsLocomotion, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsLocomotionModule, DrsLocomotion)
