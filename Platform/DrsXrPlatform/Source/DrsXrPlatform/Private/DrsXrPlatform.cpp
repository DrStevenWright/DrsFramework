// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatform.h"
#include "DrsXrArbiter.h"
#include "DrsXrDelegates.h"
#include "DrsXrDeviceInfo.h"
#include "Features/IModularFeatures.h"
#include "IHeadMountedDisplayModule.h"
#include "IOpenXRExtensionPlugin.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsXrPlatform);

IMPLEMENT_MODULE(FDrsXrPlatformModule, DrsXrPlatform)

FDrsXrDelegates::FHmdWornStateChanged FDrsXrDelegates::OnHmdWornStateChanged;
FDrsXrDelegates::FSystemMenuStateChanged FDrsXrDelegates::OnSystemMenuStateChanged;
FDrsXrDelegates::FFocusStateChanged FDrsXrDelegates::OnXrFocusStateChanged;
FDrsXrDelegates::FControllerTypeChanged FDrsXrDelegates::OnControllerTypeChanged;

namespace
{
	void UnregisterModularFeatures(FName name)
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
	// Created before the XR tracking system, so that it registers with OpenXRHMD as an extension plugin.
	if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
	{
		DeviceInfo = MakeShared<FDrsXrDeviceInfo>();
	}
#endif

	FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FDrsXrPlatformModule::OnPostEngineInit);
}

void FDrsXrPlatformModule::ShutdownModule()
{
	FCoreDelegates::GetOnPostEngineInit().RemoveAll(this);
	DeviceInfo.Reset();
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
