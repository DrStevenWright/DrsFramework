// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPassthroughBackend.h"

#if DRS_PASSTHROUGH_SUPPORTED
#include "DrsLog.h"
#include "DrsXrPassthroughSubsystem.h"
#include "DrsXrPlatformLibrary.h"
#include "Engine/Engine.h"
#include "Features/IModularFeatures.h"
#include "HAL/IConsoleManager.h"
#include "IOpenXRHMD.h"
#include "IOpenXRHMDModule.h"
#include "IXRTrackingSystem.h"
#include "RenderUtils.h"
#include "RHIGlobals.h"

namespace
{
	IOpenXRHMD* GetOpenXrHmd()
	{
		return GEngine && GEngine->XRSystem.IsValid() ? GEngine->XRSystem->GetIOpenXRHMD() : nullptr;
	}
}

FDrsXrPassthroughBackend::FDrsXrPassthroughBackend()
{
	if (!UDrsXrPlatformLibrary::IsNonVrMode())
	{
		RegisterOpenXRExtensionModularFeature();
	}
}

FDrsXrPassthroughBackend::~FDrsXrPassthroughBackend()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
}

void FDrsXrPassthroughBackend::PostCreateSession(XrSession inSession)
{
	bIsSupported = DetermineSupport();
	DRS_LOG(LogDrsXrMixedReality, Log, "Passthrough supported: %s.", *LexToString(bIsSupported));
	ApplyBlendMode();
}

void FDrsXrPassthroughBackend::OnDestroySession(XrSession inSession)
{
	bIsSupported = false;
}

void FDrsXrPassthroughBackend::SetPassthroughVisible(bool isVisible)
{
	bIsVisible = isVisible;
	ApplyBlendMode();
}

bool FDrsXrPassthroughBackend::DetermineSupport()
{
	IOpenXRHMD* openXrHmd = GetOpenXrHmd();
	if (!openXrHmd)
	{
		DRS_LOG(LogDrsXrMixedReality, Warning, "Passthrough not supported: the XR system is not OpenXR.");
		return false;
	}
	if (!openXrHmd->GetSupportedEnvironmentBlendModes().Contains(XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND))
	{
		DRS_LOG(LogDrsXrMixedReality, Log, "Passthrough not supported: the OpenXR runtime has no XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND.");
		return false;
	}
	if (!IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_COMPOSITION_LAYER_INVERTED_ALPHA_EXTENSION_NAME))
	{
		DRS_LOG(LogDrsXrMixedReality, Log, "Passthrough not supported: the OpenXR runtime has no XR_EXT_composition_layer_inverted_alpha.");
		return false;
	}

	// Project configuration errors, rather than device limits.
	static const TConsoleVariableData<bool>* invertAlphaVariable = IConsoleManager::Get().FindTConsoleVariableDataBool(TEXT("xr.OpenXRInvertAlpha"));
	if (!invertAlphaVariable || !invertAlphaVariable->GetValueOnAnyThread())
	{
		DRS_LOG(LogDrsXrMixedReality, Warning, "Passthrough not supported: enable xr.OpenXRInvertAlpha (OpenXR project settings).");
		return false;
	}
	if (!IsMobilePropagateAlphaEnabled(GMaxRHIShaderPlatform))
	{
		DRS_LOG(LogDrsXrMixedReality, Warning, "Passthrough not supported: enable r.Mobile.PropagateAlpha (Rendering project settings).");
		return false;
	}

	return true;
}

void FDrsXrPassthroughBackend::ApplyBlendMode() const
{
	IOpenXRHMD* openXrHmd = bIsSupported ? GetOpenXrHmd() : nullptr;
	if (!openXrHmd)
	{
		return;
	}

	const XrEnvironmentBlendMode blendMode = bIsVisible ? XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND : XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	if (openXrHmd->GetEnvironmentBlendMode() != blendMode)
	{
		DRS_LOG(LogDrsXrMixedReality, Log, "Passthrough %s.", bIsVisible ? TEXT("shown") : TEXT("hidden"));
		openXrHmd->SetEnvironmentBlendMode(blendMode);
	}
}
#endif
