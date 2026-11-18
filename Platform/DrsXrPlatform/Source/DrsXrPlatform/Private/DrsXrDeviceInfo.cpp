// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrDeviceInfo.h"

#include "DrsXrPlatform/DrsXrDelegates.h"
#include "DrsXrPlatform/DrsXrHandTrackingProvider.h"
#include "DrsXrPlatform/DrsXrPlatform.h"
#include "DrsXrPlatform/DrsXrPlatformLibrary.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"

#include "Features/IModularFeatures.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IHandTracker.h"
#include "IMotionController.h"
#include "Interfaces/IPluginManager.h"
#include "IOpenXRHMDModule.h"
#include "OpenXRCore.h"

FDrsXrDeviceInfo::FDrsXrDeviceInfo()
{
	if (!Drs::XrArbiter::IsNonVrMode())
	{
		RegisterOpenXRExtensionModularFeature();
	}

	// The arbiter loads at most one provider (for the selected vendor), possibly after this object is created.
	IModularFeatures& modularFeatures = IModularFeatures::Get();
	const FName providerFeatureName = IDrsXrHandTrackingProvider::GetModularFeatureName();
	if (modularFeatures.IsModularFeatureAvailable(providerFeatureName))
	{
		OnModularFeatureRegistered(providerFeatureName, &modularFeatures.GetModularFeature<IDrsXrHandTrackingProvider>(providerFeatureName));
	}
	modularFeatures.OnModularFeatureRegistered().AddRaw(this, &FDrsXrDeviceInfo::OnModularFeatureRegistered);
	modularFeatures.OnModularFeatureUnregistered().AddRaw(this, &FDrsXrDeviceInfo::OnModularFeatureUnregistered);
}

FDrsXrDeviceInfo::~FDrsXrDeviceInfo()
{
	IModularFeatures::Get().OnModularFeatureRegistered().RemoveAll(this);
	IModularFeatures::Get().OnModularFeatureUnregistered().RemoveAll(this);
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
}

void FDrsXrDeviceInfo::OnModularFeatureRegistered(const FName& type, IModularFeature* modularFeature)
{
	if (type == IDrsXrHandTrackingProvider::GetModularFeatureName())
	{
		HandTrackingProvider = static_cast<IDrsXrHandTrackingProvider*>(modularFeature);
		DRS_LOG(LogDrsXrPlatform, Log, "Using the %s hand-tracking provider.", *HandTrackingProvider->GetName());
	}
}

void FDrsXrDeviceInfo::OnModularFeatureUnregistered(const FName& type, IModularFeature* modularFeature)
{
	if (modularFeature == HandTrackingProvider)
	{
		HandTrackingProvider = nullptr;
	}
}

//------------------------------------------------------------------
// State
//------------------------------------------------------------------

const FDrsXrControllerType& FDrsXrDeviceInfo::GetControllerType()
{
	if (!ControllerType.IsValid())
	{
		// Until the runtime reports controller profiles, fall back to the HMD device name.
		ControllerType.DeviceName = UDrsXrPlatformLibrary::GetHmdDeviceName();
	}
	return ControllerType;
}

bool FDrsXrDeviceInfo::IsHandTrackingAvailable() const
{
	if (!HandTrackingAvailable.IsSet())
	{
		HandTrackingAvailable = DetermineHandTrackingAvailability();
	}
	return HandTrackingAvailable.Get(false);
}

void FDrsXrDeviceInfo::SetHmdWorn(bool isWorn)
{
	if (isWorn != bIsHmdWorn)
	{
		DRS_LOG(LogDrsXrPlatform, Log, "HMD worn: %d.", isWorn);
		bIsHmdWorn = isWorn;
		FDrsXrDelegates::OnHmdWornStateChanged.Broadcast(isWorn);
		UpdateFocus();
	}
}

void FDrsXrDeviceInfo::SetSystemMenuOpen(bool isOpen)
{
	if (isOpen != bIsSystemMenuOpen)
	{
		DRS_LOG(LogDrsXrPlatform, Log, "System menu open: %d.", isOpen);
		bIsSystemMenuOpen = isOpen;
		FDrsXrDelegates::OnSystemMenuStateChanged.Broadcast(isOpen);
		UpdateFocus();
	}
}

void FDrsXrDeviceInfo::SetControllerStates(const FDrsXrControllerState& left, const FDrsXrControllerState& right)
{
	FDrsXrControllerType controllerType = ControllerType;
	controllerType.DeviceName = UDrsXrPlatformLibrary::GetHmdDeviceName();
	controllerType.Left = left;
	controllerType.Right = right;

	if (controllerType != ControllerType)
	{
		ControllerType = controllerType;
		DRS_LOG(LogDrsXrPlatform, Log, "Controller type: %s.", *ControllerType.ToString());
		FDrsXrDelegates::OnControllerTypeChanged.Broadcast(ControllerType);
	}
}

void FDrsXrDeviceInfo::UpdateFocus()
{
	const bool hasFocus = bIsHmdWorn && !bIsSystemMenuOpen;
	if (hasFocus != bHasFocus)
	{
		DRS_LOG(LogDrsXrPlatform, Log, "XR focus: %d.", hasFocus);
		bHasFocus = hasFocus;
		FDrsXrDelegates::OnXrFocusStateChanged.Broadcast(hasFocus);
	}
}

//------------------------------------------------------------------
// Tick and OpenXR callbacks
//------------------------------------------------------------------

void FDrsXrDeviceInfo::Tick(float deltaTime)
{
	if (Session == XR_NULL_HANDLE)
	{
		return; // Wait for the session.
	}

	if (ControllerMonitor.Update(deltaTime))
	{
		SetControllerStates(ControllerMonitor.GetState(EControllerHand::Left), ControllerMonitor.GetState(EControllerHand::Right));
	}
}

bool FDrsXrDeviceInfo::GetOptionalExtensions(TArray<const ANSICHAR*>& outExtensions)
{
	outExtensions.Add(XR_EXT_USER_PRESENCE_EXTENSION_NAME); // HMD worn state, where supported.
	outExtensions.Add(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME); // Display refresh rate control, where supported.
	return true;
}

void FDrsXrDeviceInfo::OnEvent(XrSession inSession, const XrEventDataBaseHeader* inHeader)
{
	if (inHeader->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
	{
		const XrEventDataSessionStateChanged& sessionState = reinterpret_cast<const XrEventDataSessionStateChanged&>(*inHeader);
		DRS_LOG(LogDrsXrPlatform, Log, "OpenXR session state: %s.", OpenXRSessionStateToString(sessionState.state));
		SessionState = sessionState.state;

		switch (SessionState)
		{
		case XR_SESSION_STATE_SYNCHRONIZED:
			// Synced with the runtime but not visible. (Not READY: extension plugins get events before the engine handles
			// them, so the session has not begun yet.)
			EnableMixedControllerModes();
			break;

		case XR_SESSION_STATE_FOCUSED:
			EnableMixedControllerModes(); // Retries, if it failed when synchronized.
			if (!bUserPresenceReported)
			{
				SetHmdWorn(true);
			}
			SetSystemMenuOpen(false);
			break;

		case XR_SESSION_STATE_STOPPING:
		case XR_SESSION_STATE_IDLE:
			if (!bUserPresenceReported)
			{
				SetHmdWorn(false);
			}
			break;

		default:
			// READY doesn't mean the HMD is in use. VISIBLE (e.g. with the system menu open) is handled per frame in
			// UpdateDeviceLocations.
			break;
		}
	}
	else if (inHeader->type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED)
	{
		// Only sent for profiles the app suggested bindings for. The monitor decides the modes on the next tick.
		FString leftProfile;
		FString rightProfile;
		UHeadMountedDisplayFunctionLibrary::GetCurrentInteractionProfile(EControllerHand::Left, leftProfile);
		UHeadMountedDisplayFunctionLibrary::GetCurrentInteractionProfile(EControllerHand::Right, rightProfile);
		ControllerMonitor.SetProfiles(leftProfile, rightProfile);
	}
	else if (inHeader->type == XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT)
	{
		// The accurate worn state, where supported. From now on it replaces the session-state fallback.
		const XrEventDataUserPresenceChangedEXT& presenceChanged = reinterpret_cast<const XrEventDataUserPresenceChangedEXT&>(*inHeader);
		DRS_LOG(LogDrsXrPlatform, Log, "OpenXR user present: %s.", *LexToString(presenceChanged.isUserPresent == XR_TRUE));
		bUserPresenceReported = true;
		SetHmdWorn(presenceChanged.isUserPresent == XR_TRUE);
	}
	else if (inHeader->type == XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB)
	{
		const XrEventDataDisplayRefreshRateChangedFB& refreshRateChanged = reinterpret_cast<const XrEventDataDisplayRefreshRateChangedFB&>(*inHeader);
		DRS_LOG(LogDrsXrPlatform, Log, "OpenXR display refresh rate changed from %.1f Hz to %.1f Hz.", refreshRateChanged.fromDisplayRefreshRate, refreshRateChanged.toDisplayRefreshRate);
		CurrentDisplayRefreshRate = refreshRateChanged.toDisplayRefreshRate;
	}
}

void FDrsXrDeviceInfo::PostCreateInstance(XrInstance inInstance)
{
	bDisplayRefreshRateSupported = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME)
		&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrEnumerateDisplayRefreshRatesFB", reinterpret_cast<PFN_xrVoidFunction*>(&EnumerateDisplayRefreshRates)))
		&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrGetDisplayRefreshRateFB", reinterpret_cast<PFN_xrVoidFunction*>(&GetDisplayRefreshRate)))
		&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrRequestDisplayRefreshRateFB", reinterpret_cast<PFN_xrVoidFunction*>(&RequestDisplayRefreshRate)));
	DRS_LOG(LogDrsXrPlatform, Log, "OpenXR display refresh rate control supported: %s.", *LexToString(bDisplayRefreshRateSupported));
}

const void* FDrsXrDeviceInfo::OnCreateSession(XrInstance inInstance, XrSystemId inSystem, const void* inNext)
{
	// An enabled extension doesn't mean the device supports it, so check the system properties too.
	bUserPresenceSupported = false;
	bUserPresenceReported = false;
	if (IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_USER_PRESENCE_EXTENSION_NAME))
	{
		XrSystemUserPresencePropertiesEXT userPresenceProperties = { XR_TYPE_SYSTEM_USER_PRESENCE_PROPERTIES_EXT };
		XrSystemProperties systemProperties = { XR_TYPE_SYSTEM_PROPERTIES, &userPresenceProperties };
		if (XR_ENSURE(xrGetSystemProperties(inInstance, inSystem, &systemProperties)))
		{
			bUserPresenceSupported = (userPresenceProperties.supportsUserPresence == XR_TRUE);
		}
	}
	DRS_LOG(LogDrsXrPlatform, Log, "OpenXR user presence (XR_EXT_user_presence) supported: %s.", *LexToString(bUserPresenceSupported));
	return inNext;
}

void FDrsXrDeviceInfo::PostCreateSession(XrSession inSession)
{
	Session = inSession;
	SupportedDisplayRefreshRates.Reset();
	CurrentDisplayRefreshRate = 0.0f;

	if (bDisplayRefreshRateSupported)
	{
		uint32 count = 0;
		if (XR_ENSURE(EnumerateDisplayRefreshRates(Session, 0, &count, nullptr)))
		{
			SupportedDisplayRefreshRates.SetNumZeroed(count);
			const bool hasRates = XR_ENSURE(EnumerateDisplayRefreshRates(Session, count, &count, SupportedDisplayRefreshRates.GetData()));
			SupportedDisplayRefreshRates.SetNum(hasRates ? count : 0);
		}
		XR_ENSURE(GetDisplayRefreshRate(Session, &CurrentDisplayRefreshRate));

		DRS_LOG(LogDrsXrPlatform, Log, "OpenXR display refresh rate: %.1f Hz (supported: %s).", CurrentDisplayRefreshRate,
			*FString::JoinBy(SupportedDisplayRefreshRates, TEXT(", "), [](float rate) { return FString::Printf(TEXT("%.1f"), rate); }));
	}
}

void FDrsXrDeviceInfo::OnDestroySession(XrSession inSession)
{
	Session = XR_NULL_HANDLE;
	CurrentDisplayRefreshRate = 0.0f;
	SupportedDisplayRefreshRates.Reset();
	ControllerMonitor.Reset(); // The profiles are reported again once the next session attaches its action sets.
	MixedControllerModesEnabled.Reset();
	HandTrackingAvailable.Reset(); // The next session may be on another device (e.g. VR Preview in the editor).
}

void FDrsXrDeviceInfo::UpdateDeviceLocations(XrSession inSession, XrTime displayTime, XrSpace trackingSpace)
{
	// Opening the system menu and taking the HMD off pass through the same session states. This is still called each
	// frame while the system menu is open, but not while the HMD is off, which tells them apart.
	constexpr int32 systemMenuMinFrames = 3;
	if (SessionState != XR_SESSION_STATE_VISIBLE)
	{
		SystemMenuFrameCount = 0;
	}
	else if (++SystemMenuFrameCount >= systemMenuMinFrames)
	{
		SetSystemMenuOpen(true);
	}
}

void FDrsXrDeviceInfo::EnableMixedControllerModes()
{
	if (MixedControllerModesEnabled.IsSet())
	{
		return;
	}

	if (!(HandTrackingProvider && IsHandTrackingAvailable() && HandTrackingProvider->AreMixedControllerModesSupported()))
	{
		DRS_CLOG(HandTrackingProvider != nullptr, LogDrsXrPlatform, Log, "Mixed controller modes not supported (%s).", *HandTrackingProvider->GetName());
		MixedControllerModesEnabled = false;
		return;
	}

	if (HandTrackingProvider->EnableMixedControllerModes())
	{
		DRS_LOG(LogDrsXrPlatform, Log, "Mixed controller modes enabled (%s).", *HandTrackingProvider->GetName());
		MixedControllerModesEnabled = true;
	}
	else
	{
		// Left unset, so it is retried when the session is focused. Meanwhile both hands share one mode.
		DRS_LOG(LogDrsXrPlatform, Warning, "Failed to enable mixed controller modes (%s).", *HandTrackingProvider->GetName());
	}
}

//------------------------------------------------------------------
// Hand tracking and controllers
//------------------------------------------------------------------

bool FDrsXrDeviceInfo::HasTrackedHandAimPose() const
{
	// The generic OpenXR aim pose is unusable for tracked hands (at the time of writing), so only a provider's counts.
	return HandTrackingProvider && IsHandTrackingAvailable() && HandTrackingProvider->HasAimPose();
}

bool FDrsXrDeviceInfo::GetTrackedHandAimPose(EControllerHand hand, FTransform& outWorldTransform) const
{
	return HandTrackingProvider && HandTrackingProvider->GetAimPose(hand, outWorldTransform);
}

bool FDrsXrDeviceInfo::IsHandTrackingActive(EControllerHand hand) const
{
	return (hand == EControllerHand::AnyHand) ? ControllerMonitor.IsAnyHandTracked() : (ControllerMonitor.GetState(hand).Mode == EDrsXrControllerMode::TrackedHand);
}

bool FDrsXrDeviceInfo::IsTrackedHandJointTracked(EControllerHand hand) const
{
	const IHandTracker* handTracker = GetHandTracker();
	if (handTracker == nullptr || !handTracker->IsHandTrackingStateValid())
	{
		return false;
	}

	// Not GetKeypointState(): it returns whether the hand has ever been tracked (the engine keeps returning the last
	// pose), whereas the out parameter here says whether it is tracked this frame (XrHandJointLocationsEXT::isActive).
	bool isTracked = false;
	return handTracker->GetAllKeypointStates(hand, JointPositions, JointRotations, JointRadii, isTracked) && isTracked;
}

bool FDrsXrDeviceInfo::IsPhysicalControllerGripTracked(EControllerHand hand) const
{
	const IMotionController* motionController = GetMotionController();
	return motionController && motionController->GetControllerTrackingStatus(0, UDrsXrPlatformLibrary::GetGripSource(hand)) == ETrackingStatus::Tracked;
}

TOptional<bool> FDrsXrDeviceInfo::DetermineHandTrackingAvailability() const
{
	const TSharedPtr<IPlugin> handTrackingPlugin = IPluginManager::Get().FindPlugin(TEXT("OpenXRHandTracking"));
	if (!(handTrackingPlugin.IsValid() && handTrackingPlugin->IsEnabled()))
	{
		DRS_LOG(LogDrsXrPlatform, Log, "Hand tracking not available: the OpenXRHandTracking plugin is not enabled.");
		return false;
	}

	const TOptional<bool> isSupported = IsHandTrackingSupportedForDevice();
	if (!isSupported.IsSet())
	{
		return {}; // Not known until the OpenXR session exists; ask again on the next call.
	}
	if (!isSupported.GetValue())
	{
		DRS_LOG(LogDrsXrPlatform, Log, "Hand tracking not available: the OpenXR runtime or device does not support it.");
		return false;
	}
	if (HandTrackingProvider == nullptr)
	{
		DRS_LOG(LogDrsXrPlatform, Log, "Hand tracking not available: no hand-tracking provider for the selected XR plugin.");
		return false;
	}
	if (!HandTrackingProvider->IsHandTrackingEnabled())
	{
		DRS_LOG(LogDrsXrPlatform, Log, "Hand tracking not available: disabled by the %s provider.", *HandTrackingProvider->GetName());
		return false;
	}

	DRS_LOG(LogDrsXrPlatform, Log, "Hand tracking available (%s).", *HandTrackingProvider->GetName());
	return true;
}

TOptional<bool> FDrsXrDeviceInfo::IsHandTrackingSupportedForDevice() const
{
	if (Session == XR_NULL_HANDLE)
	{
		return {};
	}

	// The engine's OpenXRHandTracking plugin needs XR_EXT_hand_tracking, and checks the device's support for it
	// (XrSystemHandTrackingPropertiesEXT::supportsHandTracking) when the session is created.
	if (IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_HAND_TRACKING_EXTENSION_NAME))
	{
		if (const IHandTracker* handTracker = GetHandTracker())
		{
			return handTracker->IsHandTrackingStateValid();
		}
	}
	return false;
}

const IHandTracker* FDrsXrDeviceInfo::GetHandTracker() const
{
	if (CachedHandTracker == nullptr)
	{
		static const FName openXrHandTrackerName(TEXT("OpenXRHandTracking"));
		for (const IHandTracker* handTracker : IModularFeatures::Get().GetModularFeatureImplementations<IHandTracker>(IHandTracker::GetModularFeatureName()))
		{
			if (handTracker && handTracker->GetHandTrackerDeviceTypeName() == openXrHandTrackerName)
			{
				CachedHandTracker = handTracker;
				break;
			}
		}
	}
	return CachedHandTracker;
}

const IMotionController* FDrsXrDeviceInfo::GetMotionController() const
{
	if (CachedMotionController == nullptr)
	{
		static const FName openXrMotionControllerName(TEXT("OpenXR"));
		for (const IMotionController* motionController : IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName()))
		{
			if (motionController && motionController->GetMotionControllerDeviceTypeName() == openXrMotionControllerName)
			{
				CachedMotionController = motionController;
				break;
			}
		}
	}
	return CachedMotionController;
}

//------------------------------------------------------------------
// Display refresh rate
//------------------------------------------------------------------

float FDrsXrDeviceInfo::GetDisplayFrequency() const
{
	check(IsInGameThread());
	return CurrentDisplayRefreshRate;
}

bool FDrsXrDeviceInfo::SetDisplayFrequency(float frequency)
{
	check(IsInGameThread());

	if (Session == XR_NULL_HANDLE)
	{
		DRS_LOG(LogDrsXrPlatform, Warning, "Cannot set the display refresh rate to %.1f Hz: no OpenXR session.", frequency);
		return false;
	}
	if (!bDisplayRefreshRateSupported)
	{
		DRS_LOG(LogDrsXrPlatform, Warning, "Cannot set the display refresh rate to %.1f Hz: XR_FB_display_refresh_rate not supported.", frequency);
		return false;
	}
	if (SupportedDisplayRefreshRates.IsEmpty())
	{
		DRS_LOG(LogDrsXrPlatform, Warning, "Cannot set the display refresh rate to %.1f Hz: no supported rates reported.", frequency);
		return false;
	}

	// A rate the runtime didn't list fails, so request the closest supported one.
	float closestRate = SupportedDisplayRefreshRates[0];
	for (const float rate : SupportedDisplayRefreshRates)
	{
		if (FMath::Abs(rate - frequency) < FMath::Abs(closestRate - frequency))
		{
			closestRate = rate;
		}
	}

	DRS_CLOG(!FMath::IsNearlyEqual(closestRate, frequency, 0.5f), LogDrsXrPlatform, Warning, "Display refresh rate %.1f Hz is not supported; using the closest, %.1f Hz.", frequency, closestRate);
	DRS_LOG(LogDrsXrPlatform, Log, "Requesting the OpenXR display refresh rate %.1f Hz.", closestRate);

	const XrResult result = RequestDisplayRefreshRate(Session, closestRate);
	if (XR_FAILED(result)) // Not an ensure: some runtimes refuse (e.g. PC streaming runtimes).
	{
		DRS_LOG(LogDrsXrPlatform, Warning, "OpenXR refused the display refresh rate %.1f Hz: %s.", closestRate, OpenXRResultToString(result));
		return false;
	}
	return true;
}
