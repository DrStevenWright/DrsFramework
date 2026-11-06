// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrRuntime.h"
#include "DrsXrRuntimeUtils.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsXrRuntime);

IMPLEMENT_MODULE(FDrsXrRuntimeModule, DrsXrRuntime)

void FDrsXrRuntimeModule::StartupModule()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("DrsKeepXrRuntime")))
	{
		UE_LOG(LogDrsXrRuntime, Log, TEXT("-DrsKeepXrRuntime given; leaving the OpenXR runtime selection alone."));
		return;
	}

	const FString RuntimeJson = Drs::XrRuntime::FindOculusRuntimeJson();
	if (RuntimeJson.IsEmpty())
	{
		UE_LOG(LogDrsXrRuntime, Log, TEXT("Oculus OpenXR runtime not installed; using the system's active OpenXR runtime."));
		return;
	}

	const FString Previous = FPlatformMisc::GetEnvironmentVariable(Drs::XrRuntime::RuntimeJsonVariable);
	if (!Previous.IsEmpty() && Previous != RuntimeJson)
	{
		UE_LOG(LogDrsXrRuntime, Log, TEXT("Overriding %s, which was '%s'."), Drs::XrRuntime::RuntimeJsonVariable, *Previous);
	}

	Drs::XrRuntime::SetRuntimeJson(RuntimeJson);
	SelectedRuntimeJson = RuntimeJson;
	UE_LOG(LogDrsXrRuntime, Log, TEXT("Selected the Oculus OpenXR runtime: %s"), *RuntimeJson);
}
