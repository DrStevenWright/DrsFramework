// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsSpawning.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsSpawning);

void FDrsSpawningModule::StartupModule()
{
	DRS_LOG(LogDrsSpawning, Verbose, TEXT("Module started"));
}

void FDrsSpawningModule::ShutdownModule()
{
	DRS_LOG(LogDrsSpawning, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsSpawningModule, DrsSpawning)
