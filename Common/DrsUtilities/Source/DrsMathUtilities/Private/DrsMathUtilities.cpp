// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMathUtilities.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsMathUtilities);

void FDrsMathUtilitiesModule::StartupModule()
{
	DRS_LOG(LogDrsMathUtilities, Verbose, TEXT("Module started"));
}

void FDrsMathUtilitiesModule::ShutdownModule()
{
	DRS_LOG(LogDrsMathUtilities, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsMathUtilitiesModule, DrsMathUtilities)
