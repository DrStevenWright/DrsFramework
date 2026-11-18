// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatform/DrsXrPlatformLibrary.h"

#include "DrsXrDeviceInfo.h"
#include "DrsXrPlatform/DrsXrConfiguration.h"
#include "DrsXrPlatform/DrsXrPlatform.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"

#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "Engine/Engine.h"
#include "Features/IModularFeatures.h"
#include "HAL/IConsoleManager.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IHandTracker.h"
#include "IHeadMountedDisplay.h"
#include "IMotionController.h"
#include "ISpectatorScreenController.h"
#include "IXRTrackingSystem.h"
#include "Misc/CommandLine.h"
#include "MotionControllerComponent.h"
#include "OpenXRHandTrackingSettings.h"
#include "StereoRendering.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#if DRS_ENABLE_META_XR
#include "OculusXRFunctionLibrary.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrPlatformLibrary)

namespace
{
	const FName OpenXrMotionControllerName(TEXT("OpenXR")); // The engine's OpenXR motion controller device.
	bool bTrackingOriginRequested = false;

	FDrsXrDeviceInfo* GetDeviceInfo()
	{
		const FDrsXrPlatformModule* module = FDrsXrPlatformModule::Get();
		return module ? module->GetDeviceInfo() : nullptr;
	}

	/** Calls a device-info method, or returns the default where there is no device info (servers, commandlets). */
	template <typename FunctionType, typename ResultType>
	ResultType QueryDeviceInfo(FunctionType&& function, ResultType defaultValue)
	{
		FDrsXrDeviceInfo* deviceInfo = GetDeviceInfo();
		return deviceInfo ? function(*deviceInfo) : defaultValue;
	}

	void SetConsoleVariable(const TCHAR* name, int32 value)
	{
		if (IConsoleVariable* variable = IConsoleManager::Get().FindConsoleVariable(name))
		{
			variable->Set(value);
		}
	}
}

//------------------------------------------------------------------
// XR system and modes
//------------------------------------------------------------------

bool UDrsXrPlatformLibrary::IsXrSystemActive()
{
	return GEngine && GEngine->XRSystem.IsValid();
}

FName UDrsXrPlatformLibrary::GetXrSystemName()
{
	return IsXrSystemActive() ? GEngine->XRSystem->GetSystemName() : NAME_None;
}

bool UDrsXrPlatformLibrary::IsNonVrMode()
{
	return Drs::XrArbiter::IsNonVrMode();
}

EDrsXrPluginType UDrsXrPlatformLibrary::GetXrPluginType()
{
	return Drs::XrArbiter::GetXrPluginType();
}

bool UDrsXrPlatformLibrary::IsMetaXrMode()
{
	return GetXrPluginType() == EDrsXrPluginType::Meta;
}

bool UDrsXrPlatformLibrary::IsPicoXrMode()
{
	return GetXrPluginType() == EDrsXrPluginType::Pico;
}

EDrsXrRuntimeType UDrsXrPlatformLibrary::GetXrRuntimeType()
{
	return IsRunningDedicatedServer() ? EDrsXrRuntimeType::None : Drs::XrArbiter::GetXrRuntimeType();
}

FDrsXrConfiguration UDrsXrPlatformLibrary::GetXrConfiguration()
{
	return FDrsXrConfiguration::Get();
}

bool UDrsXrPlatformLibrary::HasXrProperty(EDrsXrPlatformProperty property)
{
	if (Drs::XrArbiter::GetXrRuntimeFlags() & Drs::XrArbiter::ToRuntimeFlag(property))
	{
		return true;
	}

	switch (property)
	{
	case EDrsXrPlatformProperty::ControllerLosesFocus:
		return IsPicoXrMode();
	default:
		return false;
	}
}

bool UDrsXrPlatformLibrary::HasXrFocus()
{
	return QueryDeviceInfo([](FDrsXrDeviceInfo& info) { return info.HasFocus(); }, false);
}

bool UDrsXrPlatformLibrary::IsSystemMenuOpen()
{
	return QueryDeviceInfo([](FDrsXrDeviceInfo& info) { return info.IsSystemMenuOpen(); }, false);
}

//------------------------------------------------------------------
// HMD
//------------------------------------------------------------------

FName UDrsXrPlatformLibrary::GetHmdDeviceName()
{
	const FName name = UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName();
	static const FName badMetaDeviceName(TEXT("OculusXRHMD"));
	ensureMsgf(name != badMetaDeviceName, TEXT("Invalid HMD device name. Make sure OculusXRHMD implements GetHMDName."));
	return name;
}

FString UDrsXrPlatformLibrary::GetDeviceProfileName()
{
	return UDeviceProfileManager::Get().GetActiveDeviceProfileName();
}

bool UDrsXrPlatformLibrary::IsDeviceProfileActive(const FString& profileName)
{
	for (const UDeviceProfile* profile = UDeviceProfileManager::Get().GetActiveProfile(); profile; profile = profile->GetParentProfile())
	{
		if (profile->GetName() == profileName)
		{
			return true;
		}
	}
	return false;
}

bool UDrsXrPlatformLibrary::IsHmdConnected()
{
	return UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected();
}

bool UDrsXrPlatformLibrary::IsHmdEnabled()
{
	return UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
}

bool UDrsXrPlatformLibrary::IsHmdTracking()
{
	return UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition();
}

bool UDrsXrPlatformLibrary::IsHmdWorn()
{
	// Not GetHMDWornState(), which (at the time of writing) only reports whether a session is running.
	return QueryDeviceInfo([](FDrsXrDeviceInfo& info) { return info.IsHmdWorn(); }, false);
}

EHMDWornState::Type UDrsXrPlatformLibrary::GetHmdWornState()
{
	return IsHmdWorn() ? EHMDWornState::Worn : EHMDWornState::NotWorn;
}

void UDrsXrPlatformLibrary::EnableHmd(bool enable)
{
	UHeadMountedDisplayFunctionLibrary::EnableHMD(enable);
}

void UDrsXrPlatformLibrary::SetupHmd()
{
	static bool isHmdSetupDone = false;
	if ((isHmdSetupDone && !GIsEditor) || IsRunningDedicatedServer())
	{
		return;
	}

	if (FString(FCommandLine::Get()).Contains(TEXT("noHMD")))
	{
		DRS_LOG(LogDrsXrPlatform, Log, "HMD disabled by -noHMD.");
		EnableHmd(false);
	}
#if WITH_EDITOR
	else if (GIsEditor && GEditor && !GEditor->IsVRPreviewActive())
	{
		DRS_LOG(LogDrsXrPlatform, Log, "HMD disabled outside VR Preview.");
		EnableHmd(false);
	}
#endif
	else if (!IsXrSystemActive())
	{
		DRS_LOG(LogDrsXrPlatform, Log, "No XR system.");
	}
	else
	{
		IHeadMountedDisplay* hmd = GEngine->XRSystem->GetHMDDevice();
		IStereoRendering* stereoRendering = GEngine->XRSystem->GetStereoRenderingDevice().Get();
		if (hmd == nullptr)
		{
			DRS_LOG(LogDrsXrPlatform, Log, "No HMD found.");
		}
		else if (stereoRendering == nullptr)
		{
			DRS_LOG(LogDrsXrPlatform, Log, "No stereo rendering device found.");
		}
		else if (!hmd->IsHMDConnected())
		{
			DRS_LOG(LogDrsXrPlatform, Log, "HMD not connected.");
		}
		else
		{
			if (!hmd->IsHMDEnabled())
			{
				DRS_LOG(LogDrsXrPlatform, Log, "Enabling the HMD.");
				hmd->EnableHMD();
			}
			if (!stereoRendering->IsStereoEnabled())
			{
				DRS_LOG(LogDrsXrPlatform, Log, "Enabling stereo rendering.");
				stereoRendering->EnableStereo();
			}
			SetFloorTrackingOrigin();
		}
	}

	isHmdSetupDone = true;
}

bool UDrsXrPlatformLibrary::SetFloorTrackingOrigin()
{
	if (!IsXrSystemActive())
	{
		return false;
	}
	GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
	bTrackingOriginRequested = true;
	DRS_LOG(LogDrsXrPlatform, Log, "Requested the LocalFloor tracking origin; the active origin is %d.", static_cast<int32>(GEngine->XRSystem->GetTrackingOrigin()));
	return true;
}

bool UDrsXrPlatformLibrary::IsTrackingOriginSetup()
{
	return bTrackingOriginRequested || UHeadMountedDisplayFunctionLibrary::GetTrackingOrigin() == EHMDTrackingOrigin::LocalFloor;
}

void UDrsXrPlatformLibrary::GetHmdDevicePose(UObject* worldContext, FRotator& outRotation, FVector& outPosition, float& outWorldToMetresScale)
{
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(outRotation, outPosition);
	outWorldToMetresScale = UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(worldContext);
}

//------------------------------------------------------------------
// Motion controllers
//------------------------------------------------------------------

FDrsXrControllerType UDrsXrPlatformLibrary::GetControllerType()
{
	return QueryDeviceInfo([](FDrsXrDeviceInfo& info) { return info.GetControllerType(); }, FDrsXrControllerType());
}

bool UDrsXrPlatformLibrary::IsControllerPresent(EControllerHand hand)
{
	return QueryDeviceInfo([hand](FDrsXrDeviceInfo& info) { return info.IsControllerPresent(hand); }, false);
}

bool UDrsXrPlatformLibrary::GetGripPose(EControllerHand hand, FTransform& outWorldTransform)
{
	return GetMotionControllerPose(OpenXrMotionControllerName, GetGripSource(hand), outWorldTransform);
}

bool UDrsXrPlatformLibrary::GetAimPose(EControllerHand hand, FTransform& outWorldTransform)
{
	// The OpenXR aim pose doesn't suit tracked hands, so use the hand-tracking provider's.
	if (IsHandTrackingActive(hand) && HasTrackedHandAimPose())
	{
		return QueryDeviceInfo([hand, &outWorldTransform](FDrsXrDeviceInfo& info) { return info.GetTrackedHandAimPose(hand, outWorldTransform); }, false);
	}
	return GetMotionControllerPose(OpenXrMotionControllerName, GetAimSource(hand), outWorldTransform);
}

bool UDrsXrPlatformLibrary::GetPalmPose(EControllerHand hand, FTransform& outWorldTransform)
{
	return GetMotionControllerPose(OpenXrMotionControllerName, GetPalmSource(hand), outWorldTransform);
}

FName UDrsXrPlatformLibrary::GetGripSource(EControllerHand hand)
{
	static const FName leftSource(TEXT("LeftGrip"));
	static const FName rightSource(TEXT("RightGrip"));
	return (hand == EControllerHand::Left) ? leftSource : rightSource;
}

FName UDrsXrPlatformLibrary::GetAimSource(EControllerHand hand)
{
	static const FName leftSource(TEXT("LeftAim"));
	static const FName rightSource(TEXT("RightAim"));
	return (hand == EControllerHand::Left) ? leftSource : rightSource;
}

FName UDrsXrPlatformLibrary::GetPalmSource(EControllerHand hand)
{
	static const FName leftSource(TEXT("LeftPalm"));
	static const FName rightSource(TEXT("RightPalm"));
	return (hand == EControllerHand::Left) ? leftSource : rightSource;
}

bool UDrsXrPlatformLibrary::GetMotionControllerPose(FName deviceName, FName source, FTransform& outWorldTransform)
{
	const IXRTrackingSystem* trackingSystem = GEngine ? GEngine->XRSystem.Get() : nullptr;
	if (trackingSystem == nullptr)
	{
		return false;
	}

	for (const IMotionController* motionController : IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName()))
	{
		if (motionController && motionController->GetMotionControllerDeviceTypeName() == deviceName)
		{
			FVector position = FVector::ZeroVector;
			FRotator rotation = FRotator::ZeroRotator;
			if (!motionController->GetControllerOrientationAndPosition(0, source, rotation, position, trackingSystem->GetWorldToMetersScale()))
			{
				return false;
			}
			const FTransform trackingToWorld = trackingSystem->GetTrackingToWorldTransform();
			outWorldTransform = FTransform(trackingToWorld.TransformRotation(FQuat(rotation)), trackingToWorld.TransformPosition(position));
			return true;
		}
	}
	return false;
}

FName UDrsXrPlatformLibrary::GetMotionControllerSource(EControllerHand hand, bool isHandTracking)
{
	if (isHandTracking)
	{
		// Tracked hands follow the wrist joint of the engine's OpenXRHandTracking plugin, not the hand-interaction grip
		// pose. The source name depends on that plugin's settings.
		const UOpenXRHandTrackingSettings* settings = GetDefault<UOpenXRHandTrackingSettings>();
		if (settings && settings->bUseMoreSpecificMotionSourceNames)
		{
			static const FName leftSource(TEXT("HandTrackingLeftWrist"));
			static const FName rightSource(TEXT("HandTrackingRightWrist"));
			return (hand == EControllerHand::Left) ? leftSource : rightSource;
		}
		static const FName leftSource(TEXT("LeftWrist"));
		static const FName rightSource(TEXT("RightWrist"));
		return (hand == EControllerHand::Left) ? leftSource : rightSource;
	}

	// Physical controllers follow the palm pose, which OpenXR intends for anchoring visuals to the user's hand.
	return GetPalmSource(hand);
}

ETrackingStatus UDrsXrPlatformLibrary::GetMotionControllerTrackingStatus(const UMotionControllerComponent* controller)
{
	// For tracked hands the source is the hand-tracking wrist, which reports whether the hand is tracked.
	return controller ? controller->CurrentTrackingStatus : ETrackingStatus::NotTracked;
}

bool UDrsXrPlatformLibrary::IsPhysicalControllerActive(EControllerHand hand)
{
	return QueryDeviceInfo([hand](FDrsXrDeviceInfo& info) { return info.IsPhysicalControllerActive(hand); }, false);
}

//------------------------------------------------------------------
// Hand tracking
//------------------------------------------------------------------

bool UDrsXrPlatformLibrary::IsHandTrackingAvailable()
{
	return QueryDeviceInfo([](FDrsXrDeviceInfo& info) { return info.IsHandTrackingAvailable(); }, false);
}

bool UDrsXrPlatformLibrary::IsHandTrackingActive(EControllerHand hand)
{
	return QueryDeviceInfo([hand](FDrsXrDeviceInfo& info) { return info.IsHandTrackingActive(hand); }, false);
}

bool UDrsXrPlatformLibrary::HasTrackedHandAimPose()
{
	return QueryDeviceInfo([](FDrsXrDeviceInfo& info) { return info.HasTrackedHandAimPose(); }, false);
}

bool UDrsXrPlatformLibrary::GetHandTrackingData(EControllerHand hand, TArray<FVector>& outPositions, TArray<FQuat>& outRotations, TArray<float>& outRadii)
{
	static const FName openXrHandTrackerName(TEXT("OpenXRHandTracking"));
	for (const IHandTracker* handTracker : IModularFeatures::Get().GetModularFeatureImplementations<IHandTracker>(IHandTracker::GetModularFeatureName()))
	{
		if (handTracker && handTracker->GetHandTrackerDeviceTypeName() == openXrHandTrackerName)
		{
			bool isTracked = false;
			return handTracker->IsHandTrackingStateValid() && handTracker->GetAllKeypointStates(hand, outPositions, outRotations, outRadii, isTracked);
		}
	}
	return false;
}

//------------------------------------------------------------------
// Rendering
//------------------------------------------------------------------

void UDrsXrPlatformLibrary::SetFoveationLevel(EDrsXrFoveationLevel foveationLevel, bool isDynamic)
{
	if (IsNonVrMode() || UE_SERVER)
	{
		return;
	}

	const int32 level = static_cast<int32>(foveationLevel);
	SetConsoleVariable(TEXT("xr.OpenXRFBFoveationLevel"), level);
	SetConsoleVariable(TEXT("xr.OpenXRFBFoveationDynamic"), isDynamic ? 1 : 0);

	// The FB foveation image is only used while hardware VRS is enabled, and the vendor plugins only set r.VRS.Enable when
	// the session is created. So enable it when foveation is raised at runtime. Never disable it: other VRS users may need
	// it, and level 0 already means full-rate shading.
	if (level > 0)
	{
		IConsoleVariable* vrsVariable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VRS.Enable"));
		if (vrsVariable && vrsVariable->GetInt() == 0)
		{
			vrsVariable->Set(1);
		}
	}
}

void UDrsXrPlatformLibrary::SetDisplayFrequency(float frequency)
{
	QueryDeviceInfo([frequency](FDrsXrDeviceInfo& info) { return info.SetDisplayFrequency(frequency); }, false);
}

float UDrsXrPlatformLibrary::GetDisplayFrequency()
{
	return QueryDeviceInfo([](FDrsXrDeviceInfo& info) { return info.GetDisplayFrequency(); }, 0.0f);
}

bool UDrsXrPlatformLibrary::GetGpuUtilisation(float& outGpuUtilisation)
{
	bool isGpuAvailable = false;
	outGpuUtilisation = 0.0f;
#if DRS_ENABLE_META_XR
	if (IsMetaXrMode())
	{
		// The XR_META_performance_metrics counter, in percent.
		UOculusXRFunctionLibrary::GetGPUUtilization(isGpuAvailable, outGpuUtilisation);
		return isGpuAvailable;
	}
#endif
	DRS_LOG_ONCE(LogDrsXrPlatform, Warning, "GPU utilisation is not available on this platform.");
	return isGpuAvailable;
}

void UDrsXrPlatformLibrary::SetSpectatorScreenMode(ESpectatorScreenMode screenMode)
{
	UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenMode(screenMode);
}

ESpectatorScreenMode UDrsXrPlatformLibrary::GetSpectatorScreenMode()
{
	if (const IHeadMountedDisplay* hmd = IsXrSystemActive() ? GEngine->XRSystem->GetHMDDevice() : nullptr)
	{
		if (const ISpectatorScreenController* controller = hmd->GetSpectatorScreenController())
		{
			return controller->GetSpectatorScreenMode();
		}
	}
	return ESpectatorScreenMode::Disabled;
}
