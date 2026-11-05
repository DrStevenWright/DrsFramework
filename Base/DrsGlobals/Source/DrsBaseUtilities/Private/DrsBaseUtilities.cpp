// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsBaseUtilities.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsBaseUtilities);

void FDrsBaseUtilitiesModule::StartupModule()
{
	DRS_LOG(LogDrsBaseUtilities, Verbose, TEXT("Module started"));
}

void FDrsBaseUtilitiesModule::ShutdownModule()
{
	DRS_LOG(LogDrsBaseUtilities, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsBaseUtilitiesModule, DrsBaseUtilities)
