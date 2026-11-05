// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInteraction.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsInteraction);

void FDrsInteractionModule::StartupModule()
{
	DRS_LOG(LogDrsInteraction, Verbose, TEXT("Module started"));
}

void FDrsInteractionModule::ShutdownModule()
{
	DRS_LOG(LogDrsInteraction, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsInteractionModule, DrsInteraction)
