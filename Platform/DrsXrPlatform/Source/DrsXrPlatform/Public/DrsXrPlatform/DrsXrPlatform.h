// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleInterface.h"

DRSXRPLATFORM_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrPlatform, Log, All);

class FDrsXrDeviceInfo;

/**
 * Owns the XR device state. Loads at PostConfigInit, so that the device info can register as an OpenXR extension
 * plugin before OpenXR creates its instance. Use UDrsXrPlatformLibrary rather than this class.
 */
class DRSXRPLATFORM_API FDrsXrPlatformModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** The device info, or null on servers and in commandlets. */
	FDrsXrDeviceInfo* GetDeviceInfo() const { return DeviceInfo.Get(); }

	static FDrsXrPlatformModule* Get();

private:
	void OnPostEngineInit();

	TSharedPtr<FDrsXrDeviceInfo> DeviceInfo;
};
