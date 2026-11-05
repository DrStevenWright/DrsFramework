// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrGameSupport.h"
#include "DrsLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsXrGameSupport);

void FDrsXrGameSupportModule::StartupModule()
{
	DRS_LOG(LogDrsXrGameSupport, Verbose, TEXT("Module started"));
}

void FDrsXrGameSupportModule::ShutdownModule()
{
	DRS_LOG(LogDrsXrGameSupport, Verbose, TEXT("Module shut down"));
}

IMPLEMENT_MODULE(FDrsXrGameSupportModule, DrsXrGameSupport)
