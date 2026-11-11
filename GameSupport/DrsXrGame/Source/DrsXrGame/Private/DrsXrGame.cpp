// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrGame.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsXrGame);

void FDrsXrGameModule::StartupModule()
{
	DRS_LOG(LogDrsXrGame, Verbose, TEXT("Module started"));
}

void FDrsXrGameModule::ShutdownModule()
{
	DRS_LOG(LogDrsXrGame, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsXrGameModule, DrsXrGame)
