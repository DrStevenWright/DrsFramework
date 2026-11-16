// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrRuntime.h"
#include "DrsLog.h"
#include "DrsXrArbiter.h"
#include "DrsXrRuntimeUtils.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsXrRuntime);

IMPLEMENT_MODULE(FDrsXrRuntimeModule, DrsXrRuntime)

namespace
{
	using Drs::XrRuntime::FRuntimeCandidate;

	/** The runtimes to consider, in order of preference. */
	TArray<FRuntimeCandidate> GetCandidates()
	{
		using namespace Drs::XrRuntime;

		EDrsXrPluginOverrideType overrideType;
		switch (Drs::XrArbiter::GetXrPluginOverride(overrideType))
		{
		case EDrsXrPluginType::Meta:
			DRS_LOG(LogDrsXrRuntime, Log, "Choosing a runtime for the forced plugin type Meta.");
			// SteamVR is the fallback for Meta headsets without Link (e.g. Steam Link or Virtual Desktop).
			return { OculusRuntime(), SteamVrRuntime() };

		case EDrsXrPluginType::Pico:
			DRS_LOG(LogDrsXrRuntime, Log, "Choosing a runtime for the forced plugin type Pico.");
			// The Pico client always runs, so SteamVR comes first: if it is running, the user means to use it.
			return { SteamVrRuntime(), PicoStreamingRuntime(), PicoConnectRuntime() };

		default:
			break;
		}

		// A running Pico client strongly suggests a Pico headset. (Meta's Link software starts at boot and keeps running
		// in the background, so it suggests nothing.) SteamVR needs the Pico client running too, so it goes first.
		if (IsPicoClientRunning())
		{
			DRS_LOG(LogDrsXrRuntime, Log, "A Pico client is running; expecting a Pico headset.");
			Drs::XrArbiter::SetExpectedXrRuntimeType(EDrsXrRuntimeType::Pico);
			return { SteamVrRuntime(), PicoStreamingRuntime(), PicoConnectRuntime() };
		}

		// Prefer dedicated runtimes over multi-vendor ones.
		return { OculusRuntime(), PicoStreamingRuntime(), PicoConnectRuntime(), SteamVrRuntime() };
	}
}

void FDrsXrRuntimeModule::StartupModule()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("DrsKeepXrRuntime")))
	{
		DRS_LOG(LogDrsXrRuntime, Log, "-DrsKeepXrRuntime given; leaving the OpenXR runtime selection alone.");
		return;
	}

	const bool isNonVrMode = Drs::XrArbiter::IsNonVrMode();
	const FRuntimeCandidate* selected = nullptr;
	FString selectedJson;

	const TArray<FRuntimeCandidate> candidates = GetCandidates();
	for (const FRuntimeCandidate& candidate : candidates)
	{
		// Some runtimes are unstable without a headset.
		if (isNonVrMode && (candidate.Flags & Drs::XrArbiter::ToRuntimeFlag(EDrsXrPlatformProperty::NonVrNotSupported)))
		{
			continue;
		}

		const FString runtimeJson = candidate.FindRuntimeJson();
		if (runtimeJson.IsEmpty())
		{
			continue;
		}

		const FString runtimeName = Drs::XrArbiter::RuntimeTypeToString(candidate.Type);
		DRS_LOG(LogDrsXrRuntime, Log, "Runtime '%s' installed at '%s'.", *runtimeName, *runtimeJson);
		if (candidate.IsRunning())
		{
			DRS_LOG(LogDrsXrRuntime, Log, "Runtime '%s' is already running.", *runtimeName);
			selected = &candidate;
			selectedJson = runtimeJson;
			break;
		}
		if (selected == nullptr)
		{
			selected = &candidate;
			selectedJson = runtimeJson;
		}
	}

	if (selected == nullptr)
	{
		DRS_LOG(LogDrsXrRuntime, Log, "No known OpenXR runtime installed; using the system's active OpenXR runtime.");
		return;
	}

	const FString previousRuntimeJson = FPlatformMisc::GetEnvironmentVariable(Drs::XrRuntime::RuntimeJsonVariable);
	if (!previousRuntimeJson.IsEmpty() && previousRuntimeJson != selectedJson)
	{
		DRS_LOG(LogDrsXrRuntime, Log, "Overriding %s, which was '%s'.", Drs::XrRuntime::RuntimeJsonVariable, *previousRuntimeJson);
	}

	Drs::XrRuntime::SetRuntimeJson(selectedJson);
	Drs::XrArbiter::SetXrRuntime(selected->Type, selected->Flags);
	SelectedRuntimeJson = selectedJson;
	DRS_LOG(LogDrsXrRuntime, Log, "Selected the %s OpenXR runtime: %s", *Drs::XrArbiter::RuntimeTypeToString(selected->Type), *selectedJson);
}
