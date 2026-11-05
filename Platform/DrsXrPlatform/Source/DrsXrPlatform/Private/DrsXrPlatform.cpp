// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatform.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsXrPlatform);

void FDrsXrPlatformModule::StartupModule()
{
	DRS_LOG(LogDrsXrPlatform, Verbose, TEXT("Module started"));
}

void FDrsXrPlatformModule::ShutdownModule()
{
	DRS_LOG(LogDrsXrPlatform, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsXrPlatformModule, DrsXrPlatform)
