// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsCore.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsCore);

void FDrsCoreModule::StartupModule()
{
	DRS_LOG(LogDrsCore, Verbose, TEXT("Module started"));
}

void FDrsCoreModule::ShutdownModule()
{
	DRS_LOG(LogDrsCore, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsCoreModule, DrsCore)
