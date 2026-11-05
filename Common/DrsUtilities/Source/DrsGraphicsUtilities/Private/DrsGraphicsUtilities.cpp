// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsGraphicsUtilities.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsGraphicsUtilities);

void FDrsGraphicsUtilitiesModule::StartupModule()
{
	DRS_LOG(LogDrsGraphicsUtilities, Verbose, TEXT("Module started"));
}

void FDrsGraphicsUtilitiesModule::ShutdownModule()
{
	DRS_LOG(LogDrsGraphicsUtilities, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsGraphicsUtilitiesModule, DrsGraphicsUtilities)
