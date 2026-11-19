// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleInterface.h"

DRSXRPLATFORM_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrPlatform, Log, All);

namespace Drs::XrPlatform { class FXrDeviceExtensions; }

/**
 * Owns the XR device state (FXrDeviceExtensions, which owns FXrControllerExtensions). Loads at PostConfigInit, so that it can register as an OpenXR extension
 * plugin before OpenXR creates its instance. Use UDrsXrPlatformLibrary rather than direct access
 * to this class.
 */
class DRSXRPLATFORM_API FDrsXrPlatformModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** The device state, or null on servers and in commandlets. */
	Drs::XrPlatform::FXrDeviceExtensions* GetDeviceExtensions() const { return DeviceExtensions.Get(); }

	static FDrsXrPlatformModule* Get();

private:
	void OnPostEngineInit();

	TSharedPtr<Drs::XrPlatform::FXrDeviceExtensions> DeviceExtensions;
};
