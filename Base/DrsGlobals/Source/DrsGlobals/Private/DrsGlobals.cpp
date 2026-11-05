// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsGlobals.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsGlobals);

void FDrsGlobalsModule::StartupModule()
{
	DRS_LOG(LogDrsGlobals, Verbose, TEXT("Module started"));
}

void FDrsGlobalsModule::ShutdownModule()
{
	DRS_LOG(LogDrsGlobals, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsGlobalsModule, DrsGlobals)
