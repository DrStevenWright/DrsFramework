// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatform/DrsXrPlatformLibrary.h"

#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrArbiter/DrsXrSelection.h"
#include "DrsXrControllerExtensions.h"
#include "DrsXrDeviceExtensions.h"
#include "DrsXrPlatform/DrsXrPlatform.h"
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
#include "Misc/Parse.h"
#include "MotionControllerComponent.h"
#include "OpenXRHandTrackingSettings.h"
#include "StereoRendering.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrPlatformLibrary)

using Drs::XrArbiter::FXrSelection;
using Drs::XrPlatform::FXrControllerExtensions;
using Drs::XrPlatform::FXrDeviceExtensions;

namespace
{
	const FName OpenXrMotionControllerName(TEXT("OpenXR")); // The engine's OpenXR motion controller device.
	bool bTrackingOriginRequested = false;

	FXrDeviceExtensions* GetDeviceExtensions()
	{
		const FDrsXrPlatformModule* module = FDrsXrPlatformModule::Get();
		return module ? module->GetDeviceExtensions() : nullptr;
	}

	/** Calls a device-state method, or returns the default where there is no device state (servers, commandlets). */
	template <typename FunctionType, typename ResultType>
	ResultType QueryDeviceExtensions(FunctionType&& function, ResultType defaultValue)
	{
		FXrDeviceExtensions* deviceExtensions = GetDeviceExtensions();
		return deviceExtensions ? function(*deviceExtensions) : defaultValue;
	}

	/** Calls a hands-and-controllers method, or returns the default where there is no device state (servers, commandlets). */
	template <typename FunctionType, typename ResultType>
	ResultType QueryControllerExtensions(FunctionType&& function, ResultType defaultValue)
	{
		FXrDeviceExtensions* deviceExtensions = GetDeviceExtensions();
		return deviceExtensions ? function(deviceExtensions->GetControllerExtensions()) : defaultValue;
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

FName UDrsXrPlatformLibrary::GetXrPluginType()
{
	return Drs::XrArbiter::GetXrPluginType();
}

FName UDrsXrPlatformLibrary::GetXrRuntimeType()
{
	return IsRunningDedicatedServer() ? NAME_None : Drs::XrArbiter::GetXrRuntimeType();
}

bool UDrsXrPlatformLibrary::HasXrFocus()
{
	return QueryDeviceExtensions([](FXrDeviceExtensions& device) { return device.HasFocus(); }, false);
}

bool UDrsXrPlatformLibrary::IsSystemMenuOpen()
{
	return QueryDeviceExtensions([](FXrDeviceExtensions& device) { return device.IsSystemMenuOpen(); }, false);
}

//------------------------------------------------------------------
// HMD
//------------------------------------------------------------------

FName UDrsXrPlatformLibrary::GetHmdDeviceName()
{
	return UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName();
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
	// Not GetHMDWornState(), which (at the time of writing) only reports whether a session is running. FXrDeviceExtensions uses
	// XR_EXT_user_presence where supported, and otherwise infers it from the OpenXR session state.
	return QueryDeviceExtensions([](FXrDeviceExtensions& device) { return device.IsHmdWorn(); }, false);
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

	if (FParse::Param(FCommandLine::Get(), TEXT("noHMD")))
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

			// Validate compatibility of selected OpenXR runtime and loaded vendor XR plugins.
			(void)FXrSelection::Get().Validate();
		}
	}

	isHmdSetupDone = true;
}

bool UDrsXrPlatformLibrary::SetFloorTrackingOrigin()
{
	// Asks for OpenXR LOCAL_FLOOR; the runtime falls back to LOCAL if it doesn't support it.
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
	return QueryControllerExtensions([](FXrControllerExtensions& controllers) { return controllers.GetControllerType(); }, FDrsXrControllerType());
}

bool UDrsXrPlatformLibrary::IsControllerPresent(EControllerHand hand)
{
	return QueryControllerExtensions([hand](FXrControllerExtensions& controllers) { return controllers.IsControllerPresent(hand); }, false);
}

bool UDrsXrPlatformLibrary::GetGripPose(EControllerHand hand, FTransform& outWorldTransform)
{
	return GetMotionControllerPose(OpenXrMotionControllerName, GetGripSource(hand), outWorldTransform);
}

bool UDrsXrPlatformLibrary::GetAimPose(EControllerHand hand, FTransform& outWorldTransform)
{
	// The OpenXR aim pose doesn't suit tracked hands, so use the runtime's hand aim (XR_FB_hand_tracking_aim, through
	// FXrControllerExtensions) where it gives one, and the OpenXR aim pose otherwise.
	if (IsHandTrackingActive(hand) && HasTrackedHandAimPose())
	{
		return QueryControllerExtensions([hand, &outWorldTransform](FXrControllerExtensions& controllers) { return controllers.GetTrackedHandAimPose(hand, outWorldTransform); }, false);
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
	return QueryControllerExtensions([hand](FXrControllerExtensions& controllers) { return controllers.IsPhysicalControllerActive(hand); }, false);
}

//------------------------------------------------------------------
// Hand tracking
//------------------------------------------------------------------

bool UDrsXrPlatformLibrary::IsHandTrackingAvailable()
{
	return QueryControllerExtensions([](FXrControllerExtensions& controllers) { return controllers.IsHandTrackingAvailable(); }, false);
}

bool UDrsXrPlatformLibrary::IsHandTrackingActive(EControllerHand hand)
{
	return QueryControllerExtensions([hand](FXrControllerExtensions& controllers) { return controllers.IsHandTrackingActive(hand); }, false);
}

bool UDrsXrPlatformLibrary::HasTrackedHandAimPose()
{
	// Whether the runtime gives XR_FB_hand_tracking_aim. Without it, GetAimPose gives the OpenXR aim pose for tracked hands.
	return QueryControllerExtensions([](FXrControllerExtensions& controllers) { return controllers.HasTrackedHandAimPose(); }, false);
}

bool UDrsXrPlatformLibrary::GetHandTrackingData(EControllerHand hand, TArray<FVector>& outPositions, TArray<FQuat>& outRotations, TArray<float>& outRadii)
{
	// The raw joints from the engine's OpenXRHandTracking (XR_EXT_hand_tracking).
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
	QueryDeviceExtensions([frequency](FXrDeviceExtensions& device) { return device.SetDisplayFrequency(frequency); }, false);
}

float UDrsXrPlatformLibrary::GetDisplayFrequency()
{
	return QueryDeviceExtensions([](FXrDeviceExtensions& device) { return device.GetDisplayFrequency(); }, 0.0f);
}

bool UDrsXrPlatformLibrary::GetGpuUtilisation(float& outGpuUtilisation)
{
	// The XR_META_performance_metrics counter. Only standalone Quest reports it; Link and SteamVR give frame times only.
	outGpuUtilisation = 0.0f;
	const bool isAvailable = QueryDeviceExtensions([&outGpuUtilisation](FXrDeviceExtensions& device) { return device.GetGpuUtilisation(outGpuUtilisation); }, false);
	DRS_CLOG_ONCE(!isAvailable, LogDrsXrPlatform, Log, "GPU utilisation is not available: the OpenXR runtime doesn't report it.");
	return isAvailable;
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
