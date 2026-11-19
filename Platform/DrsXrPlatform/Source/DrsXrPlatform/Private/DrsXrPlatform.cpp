// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatform/DrsXrPlatform.h"

#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrDeviceExtensions.h"
#include "DrsXrPlatform/DrsXrDelegates.h"
#include "Features/IModularFeatures.h"
#include "IHeadMountedDisplayModule.h"
#include "IOpenXRExtensionPlugin.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"

using Drs::XrPlatform::FXrDelegates;
using Drs::XrPlatform::FXrDeviceExtensions;

DEFINE_LOG_CATEGORY(LogDrsXrPlatform);

IMPLEMENT_MODULE(FDrsXrPlatformModule, DrsXrPlatform)

FXrDelegates::FHmdWornStateChanged FXrDelegates::OnHmdWornStateChanged;
FXrDelegates::FSystemMenuStateChanged FXrDelegates::OnSystemMenuStateChanged;
FXrDelegates::FFocusStateChanged FXrDelegates::OnXrFocusStateChanged;
FXrDelegates::FControllerTypeChanged FXrDelegates::OnControllerTypeChanged;

namespace
{
	void UnregisterModularFeatures(const FName name)
	{
		IModularFeatures& modularFeatures = IModularFeatures::Get();
		while (modularFeatures.GetModularFeatureImplementationCount(name) > 0)
		{
			modularFeatures.UnregisterModularFeature(name, modularFeatures.GetModularFeatureImplementation(name, 0));
		}
	}
}

void FDrsXrPlatformModule::StartupModule()
{
#if !UE_SERVER
	// Created before the XR tracking system, so that it (and the extensions it owns) register with OpenXRHMD as extension plugins.
	if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
	{
		DeviceExtensions = MakeShared<FXrDeviceExtensions>();
	}
#endif

	FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FDrsXrPlatformModule::OnPostEngineInit);
}

void FDrsXrPlatformModule::ShutdownModule()
{
	FCoreDelegates::GetOnPostEngineInit().RemoveAll(this);
	DeviceExtensions.Reset();
}

void FDrsXrPlatformModule::OnPostEngineInit()
{
	// Without VR, unregister the OpenXR modular features, or some accessors try to initialise OpenXR on every call.
	if (Drs::XrArbiter::IsNonVrMode())
	{
		UnregisterModularFeatures(IHeadMountedDisplayModule::GetModularFeatureName());
		UnregisterModularFeatures(IOpenXRExtensionPlugin::GetModularFeatureName());
	}
}

FDrsXrPlatformModule* FDrsXrPlatformModule::Get()
{
	static const FName moduleName(TEXT("DrsXrPlatform"));
	return FModuleManager::GetModulePtr<FDrsXrPlatformModule>(moduleName);
}
