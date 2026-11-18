// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "CoreMinimal.h"
#include "DrsXrPlatform/DrsXrHandTrackingProvider.h"
#include "Engine/Engine.h"
#include "Features/IModularFeatures.h"
#include "InputCoreTypes.h"
#include "IOpenXRHMDModule.h"
#include "IXRTrackingSystem.h"
#include "Modules/ModuleManager.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "OculusXRInputFunctionLibrary.h"
#include <openxr/openxr.h>

using Drs::XrPlatform::IXrHandTrackingProvider;

DEFINE_LOG_CATEGORY_STATIC(LogDrsXrHandTrackingMeta, Log, All);

namespace
{
	/** Meta hand tracking, through the MetaXR plugin. The arbiter loads this module only when it selects Meta. */
	class FXrHandTrackingMetaProvider : public IXrHandTrackingProvider
	{
	public:
		virtual FString GetName() const override { return TEXT("Meta"); }

		virtual bool IsHandTrackingEnabled() const override
		{
			const UOculusXRHMDRuntimeSettings* settings = GetDefault<UOculusXRHMDRuntimeSettings>();
			if (settings->HandTrackingSupport == EOculusXRHandTrackingSupport::ControllersOnly)
			{
				UE_LOG(LogDrsXrHandTrackingMeta, Log, TEXT("Hand tracking disabled: the MetaXR Hand Tracking Support setting is Controllers Only."));
				return false;
			}
			return true;
		}

		virtual bool HasAimPose() const override
		{
			return IOpenXRHMDModule::Get().IsExtensionEnabled(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
		}

		virtual bool GetAimPose(EControllerHand hand, FTransform& outWorldTransform) const override
		{
			// The OpenXR aim pose is unusable for tracked hands on Meta (at the time of writing), so use the MetaXR pointer
			// pose (XR_FB_hand_tracking_aim), the runtime's stabilised pointing ray. Recheck with new MetaXR releases.
			const EOculusXRHandType metaHand = (hand == EControllerHand::Left) ? EOculusXRHandType::HandLeft : EOculusXRHandType::HandRight;
			const IXRTrackingSystem* trackingSystem = GEngine ? GEngine->XRSystem.Get() : nullptr;
			if (trackingSystem && UOculusXRInputFunctionLibrary::IsPointerPoseValid(metaHand))
			{
				outWorldTransform = UOculusXRInputFunctionLibrary::GetPointerPose(metaHand) * trackingSystem->GetTrackingToWorldTransform();
				return true;
			}
			return false;
		}

		virtual bool AreMixedControllerModesSupported() const override
		{
			return IOpenXRHMDModule::Get().IsExtensionEnabled(TEXT("XR_META_simultaneous_hands_and_controllers"));
		}

		virtual bool EnableMixedControllerModes() override
		{
			// MetaXR pauses simultaneous hands and controllers whenever a session is created, so it is resumed for every
			// session. MetaXR calls the extension unchecked, so check that it is supported first.
			return AreMixedControllerModesSupported() && UOculusXRInputFunctionLibrary::SetSimultaneousHandsAndControllersEnabled(true);
		}
	};

	class FDrsXrHandTrackingMetaModule : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
		{
			IModularFeatures::Get().RegisterModularFeature(IXrHandTrackingProvider::GetModularFeatureName(), &Provider);
		}

		virtual void ShutdownModule() override
		{
			IModularFeatures::Get().UnregisterModularFeature(IXrHandTrackingProvider::GetModularFeatureName(), &Provider);
		}

	private:
		FXrHandTrackingMetaProvider Provider;
	};
}

IMPLEMENT_MODULE(FDrsXrHandTrackingMetaModule, DrsXrHandTrackingMeta)
