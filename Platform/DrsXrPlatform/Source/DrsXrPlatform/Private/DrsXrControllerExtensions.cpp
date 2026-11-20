// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrControllerExtensions.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrPlatform/DrsXrDelegates.h"
#include "DrsXrPlatform/DrsXrPlatform.h"
#include "DrsXrPlatform/DrsXrPlatformLibrary.h"
#include "DrsXrPlatform/DrsXrPlatformSettings.h"
#include "Engine/Engine.h"
#include "Features/IModularFeatures.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IHandTracker.h"
#include "IMotionController.h"
#include "InputCoreTypes.h"
#include "Interfaces/IPluginManager.h"
#include "IOpenXRHMDModule.h"
#include "IXRTrackingSystem.h"
#include "OpenXRCore.h"

namespace
{
	// XR_META_simultaneous_hands_and_controllers, which the engine's OpenXR header doesn't declare. These follow the OpenXR
	// registry, under Drs names so they can't clash with MetaXR's copy when both are in a build.
	const ANSICHAR* SimultaneousHandsAndControllersExtensionName = "XR_META_simultaneous_hands_and_controllers";

	/** XrSystemSimultaneousHandsAndControllersPropertiesMETA. */
	struct FXrSystemSimultaneousHandsAndControllersProperties
	{
		XrStructureType type = static_cast<XrStructureType>(1000532001);
		void* next = nullptr;
		XrBool32 supportsSimultaneousHandsAndControllers = XR_FALSE;
	};

	/** XrSimultaneousHandsAndControllersTrackingResumeInfoMETA. */
	struct FXrSimultaneousHandsAndControllersResumeInfo
	{
		XrStructureType type = static_cast<XrStructureType>(1000532002);
		const void* next = nullptr;
	};

	/** PFN_xrResumeSimultaneousHandsAndControllersTrackingMETA. */
	using FResumeSimultaneousHandsAndControllersFunction = XrResult(XRAPI_PTR*)(XrSession session, const FXrSimultaneousHandsAndControllersResumeInfo* resumeInfo);

	// Here rather than in FXrControllerExtensions, whose header can't name the type. There is only one instance.
	FResumeSimultaneousHandsAndControllersFunction ResumeSimultaneousHandsAndControllers = nullptr;

	/** The path for a string, through the new instance (FOpenXRPath needs the engine's instance, which may not be set yet). */
	XrPath ToPath(XrInstance instance, const char* pathString)
	{
		XrPath path = XR_NULL_PATH;
		XR_ENSURE(xrStringToPath(instance, pathString, &path));
		return path;
	}
}

namespace Drs::XrPlatform
{
	FXrControllerExtensions::FXrControllerExtensions()
	{
		if (!Drs::XrArbiter::IsNonVrMode())
		{
			RegisterOpenXRExtensionModularFeature();
		}
	}

	FXrControllerExtensions::~FXrControllerExtensions()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	//------------------------------------------------------------------
	// State
	//------------------------------------------------------------------

	const FDrsXrControllerType& FXrControllerExtensions::GetControllerType()
	{
		if (!ControllerType.IsValid())
		{
			// Until the runtime reports controller profiles, fall back to the HMD device name.
			ControllerType.DeviceName = UDrsXrPlatformLibrary::GetHmdDeviceName();
		}
		return ControllerType;
	}

	bool FXrControllerExtensions::IsHandTrackingAvailable() const
	{
		if (!HandTrackingAvailable.IsSet())
		{
			HandTrackingAvailable = DetermineHandTrackingAvailability();
		}
		return HandTrackingAvailable.Get(false);
	}

	void FXrControllerExtensions::SetControllerStates(const FDrsXrControllerState& left, const FDrsXrControllerState& right)
	{
		FDrsXrControllerType controllerType = ControllerType;
		controllerType.DeviceName = UDrsXrPlatformLibrary::GetHmdDeviceName();
		controllerType.Left = left;
		controllerType.Right = right;

		if (controllerType != ControllerType)
		{
			ControllerType = controllerType;
			DRS_LOG(LogDrsXrPlatform, Log, "Controller type: %s.", *ControllerType.ToString());
			FXrDelegates::OnControllerTypeChanged.Broadcast(ControllerType);
		}
	}

	//------------------------------------------------------------------
	// Tick and OpenXR callbacks
	//------------------------------------------------------------------

	void FXrControllerExtensions::Tick(float deltaTime)
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

	bool FXrControllerExtensions::GetOptionalExtensions(TArray<const ANSICHAR*>& outExtensions)
	{
		outExtensions.Add(XR_EXT_HAND_INTERACTION_EXTENSION_NAME);
		outExtensions.Add(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
		outExtensions.Add(SimultaneousHandsAndControllersExtensionName);
		return true;
	}

	void FXrControllerExtensions::PostCreateInstance(XrInstance inInstance)
	{
		const IOpenXRHMDModule& openXr = IOpenXRHMDModule::Get();

		// The aim state comes with the hand joints (XR_EXT_hand_tracking, which the engine's OpenXRHandTracking requests).
		bIsAimEnabled = openXr.IsExtensionEnabled(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME) && openXr.IsExtensionEnabled(XR_EXT_HAND_TRACKING_EXTENSION_NAME)
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrCreateHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction*>(&CreateHandTracker)))
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrDestroyHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction*>(&DestroyHandTracker)))
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrLocateHandJointsEXT", reinterpret_cast<PFN_xrVoidFunction*>(&LocateHandJoints)));

		bIsSimultaneousHandsAndControllersEnabled = openXr.IsExtensionEnabled(ANSI_TO_TCHAR(SimultaneousHandsAndControllersExtensionName))
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrResumeSimultaneousHandsAndControllersTrackingMETA", reinterpret_cast<PFN_xrVoidFunction*>(&ResumeSimultaneousHandsAndControllers)));

		DRS_LOG(LogDrsXrPlatform, Log, "OpenXR hand aim (XR_FB_hand_tracking_aim) enabled: %s. Simultaneous hands and controllers enabled: %s.",
			*LexToString(bIsAimEnabled), *LexToString(bIsSimultaneousHandsAndControllersEnabled));

		SuggestHandInteractionBindings(inInstance);
	}

	void FXrControllerExtensions::SuggestHandInteractionBindings(XrInstance instance)
	{
		HandInteractionActionSet = XR_NULL_HANDLE; // Any earlier set went with the earlier instance.

		if (!IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_HAND_INTERACTION_EXTENSION_NAME))
		{
			DRS_LOG(LogDrsXrPlatform, Log, "OpenXR hand interaction profile (XR_EXT_hand_interaction) not supported.");
			return;
		}
		if (UDrsXrPlatformSettings::GetHandTrackingSupport() == EDrsXrHandTrackingSupport::ControllersOnly)
		{
			return;
		}

		// A later xrSuggestInteractionProfileBindings call for the same profile replaces this one, and a vendor plugin may
		// suggest its own bindings for this profile (MetaXR does, for microgestures). So leave the profile to a running
		// vendor plugin, or one of the two sets of bindings would be lost.
		const FName vendorPlugin = Drs::XrArbiter::GetXrPluginType();
		if (!vendorPlugin.IsNone())
		{
			DRS_LOG(LogDrsXrPlatform, Log, "Hand interaction bindings left to the %s plugin.", *vendorPlugin.ToString());
			return;
		}

		XrActionSetCreateInfo actionSetInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
		FCStringAnsi::Strncpy(actionSetInfo.actionSetName, "drshandinteraction", XR_MAX_ACTION_SET_NAME_SIZE);
		FCStringAnsi::Strncpy(actionSetInfo.localizedActionSetName, "DRS Hand Interaction", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
		XrActionSet actionSet = XR_NULL_HANDLE;
		if (!XR_ENSURE(xrCreateActionSet(instance, &actionSetInfo, &actionSet)))
		{
			return;
		}

		// One action is enough for the runtime to report the profile. The pinch value is the one input every hand has.
		const XrPath handPaths[] = { ToPath(instance, "/user/hand/left"), ToPath(instance, "/user/hand/right") };
		XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
		actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		actionInfo.countSubactionPaths = UE_ARRAY_COUNT(handPaths);
		actionInfo.subactionPaths = handPaths;
		FCStringAnsi::Strncpy(actionInfo.actionName, "pinch", XR_MAX_ACTION_NAME_SIZE);
		FCStringAnsi::Strncpy(actionInfo.localizedActionName, "Pinch", XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
		XrAction pinchAction = XR_NULL_HANDLE;
		if (!XR_ENSURE(xrCreateAction(actionSet, &actionInfo, &pinchAction)))
		{
			xrDestroyActionSet(actionSet);
			return;
		}

		const XrActionSuggestedBinding bindings[] = {
			{ pinchAction, ToPath(instance, "/user/hand/left/input/pinch_ext/value") },
			{ pinchAction, ToPath(instance, "/user/hand/right/input/pinch_ext/value") },
		};
		XrInteractionProfileSuggestedBinding suggestedBindings = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = ToPath(instance, "/interaction_profiles/ext/hand_interaction_ext");
		suggestedBindings.countSuggestedBindings = UE_ARRAY_COUNT(bindings);
		suggestedBindings.suggestedBindings = bindings;
		if (!XR_ENSURE(xrSuggestInteractionProfileBindings(instance, &suggestedBindings)))
		{
			xrDestroyActionSet(actionSet);
			return;
		}

		HandInteractionActionSet = actionSet;
		DRS_LOG(LogDrsXrPlatform, Log, "Suggested hand interaction bindings (XR_EXT_hand_interaction).");
	}

	const void* FXrControllerExtensions::OnCreateSession(XrInstance inInstance, XrSystemId inSystem, const void* inNext)
	{
		// An enabled extension doesn't mean the device supports it, so check the system properties too.
		XrSystemHandTrackingPropertiesEXT handTrackingProperties = { XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
		FXrSystemSimultaneousHandsAndControllersProperties simultaneousProperties;
		if (bIsSimultaneousHandsAndControllersEnabled)
		{
			handTrackingProperties.next = &simultaneousProperties;
		}
		XrSystemProperties systemProperties = { XR_TYPE_SYSTEM_PROPERTIES, &handTrackingProperties };
		const bool hasProperties = XR_ENSURE(xrGetSystemProperties(inInstance, inSystem, &systemProperties));

		bIsHandTrackingSupported = hasProperties && handTrackingProperties.supportsHandTracking == XR_TRUE;
		bSimultaneousHandsAndControllersSupported = hasProperties && bIsSimultaneousHandsAndControllersEnabled
			&& simultaneousProperties.supportsSimultaneousHandsAndControllers == XR_TRUE;
		return inNext;
	}

	void FXrControllerExtensions::PostCreateSession(XrSession inSession)
	{
		Session = inSession;

		if (bIsAimEnabled && bIsHandTrackingSupported)
		{
			const XrHandEXT hands[] = { XR_HAND_LEFT_EXT, XR_HAND_RIGHT_EXT };
			for (int32 handIndex = 0; handIndex < 2; ++handIndex)
			{
				XrHandTrackerCreateInfoEXT createInfo = { XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
				createInfo.hand = hands[handIndex];
				createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
				if (!XR_ENSURE(CreateHandTracker(Session, &createInfo, &HandTrackers[handIndex])))
				{
					DestroyHandTrackers();
					break;
				}
			}
		}
	}

	void FXrControllerExtensions::OnDestroySession(XrSession inSession)
	{
		DestroyHandTrackers();
		Session = XR_NULL_HANDLE;
		ControllerMonitor.Reset(); // The profiles are reported again once the next session attaches its action sets.
		MixedControllerModesEnabled.Reset();
		HandTrackingAvailable.Reset(); // The next session may be on another device (e.g. VR Preview in the editor).
	}

	void FXrControllerExtensions::OnEvent(XrSession inSession, const XrEventDataBaseHeader* inHeader)
	{
		if (inHeader->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
		{
			// SYNCHRONIZED: synced with the runtime but not visible. (Not READY: extension plugins get events before the
			// engine handles them, so the session has not begun yet.) FOCUSED retries, if it failed when synchronized.
			const XrSessionState state = reinterpret_cast<const XrEventDataSessionStateChanged&>(*inHeader).state;
			if (state == XR_SESSION_STATE_SYNCHRONIZED || state == XR_SESSION_STATE_FOCUSED)
			{
				EnableMixedControllerModes();
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
	}

	void FXrControllerExtensions::UpdateDeviceLocations(XrSession inSession, XrTime displayTime, XrSpace trackingSpace)
	{
		if (HandTrackers[0] == XR_NULL_HANDLE)
		{
			return;
		}

		// The hand aim, located in tracking space with the joints.
		for (int32 handIndex = 0; handIndex < 2; ++handIndex)
		{
			XrHandTrackingAimStateFB aimState = { XR_TYPE_HAND_TRACKING_AIM_STATE_FB };
			XrHandJointLocationsEXT jointLocations = { XR_TYPE_HAND_JOINT_LOCATIONS_EXT, &aimState };
			jointLocations.jointCount = XR_HAND_JOINT_COUNT_EXT;
			jointLocations.jointLocations = JointLocations;

			XrHandJointsLocateInfoEXT locateInfo = { XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
			locateInfo.baseSpace = trackingSpace;
			locateInfo.time = displayTime;

			const bool isLocated = XR_SUCCEEDED(LocateHandJoints(HandTrackers[handIndex], &locateInfo, &jointLocations));
			bIsAimValid[handIndex] = isLocated && jointLocations.isActive && (aimState.status & XR_HAND_TRACKING_AIM_VALID_BIT_FB);
			if (bIsAimValid[handIndex])
			{
				AimPoses[handIndex] = aimState.aimPose;
			}
		}
	}

	void FXrControllerExtensions::AttachActionSets(TSet<XrActionSet>& outActionSets)
	{
		if (HandInteractionActionSet != XR_NULL_HANDLE)
		{
			outActionSets.Add(HandInteractionActionSet);
		}
	}

	void FXrControllerExtensions::GetActiveActionSetsForSync(TArray<XrActiveActionSet>& outActiveSets)
	{
		if (HandInteractionActionSet != XR_NULL_HANDLE)
		{
			outActiveSets.Add({ HandInteractionActionSet, XR_NULL_PATH });
		}
	}

	//------------------------------------------------------------------
	// Mixed controller modes
	//------------------------------------------------------------------

	void FXrControllerExtensions::EnableMixedControllerModes()
	{
		if (MixedControllerModesEnabled.IsSet())
		{
			return;
		}

		if (!(IsHandTrackingAvailable() && bSimultaneousHandsAndControllersSupported && Session != XR_NULL_HANDLE))
		{
			DRS_LOG(LogDrsXrPlatform, Log, "Mixed controller modes not supported (XR_META_simultaneous_hands_and_controllers).");
			MixedControllerModesEnabled = false;
			return;
		}

		// A vendor plugin may pause simultaneous hands and controllers when a session is created (MetaXR does, and nothing
		// else resumes it), so this runs even then. Resuming twice is harmless.
		const FXrSimultaneousHandsAndControllersResumeInfo resumeInfo;
		if (XR_SUCCEEDED(ResumeSimultaneousHandsAndControllers(Session, &resumeInfo)))
		{
			DRS_LOG(LogDrsXrPlatform, Log, "Mixed controller modes enabled.");
			MixedControllerModesEnabled = true;
		}
		else
		{
			// Left unset, so it is retried when the session is focused. Meanwhile both hands share one mode.
			DRS_LOG(LogDrsXrPlatform, Warning, "Failed to enable mixed controller modes.");
		}
	}

	//------------------------------------------------------------------
	// Hand tracking and controllers
	//------------------------------------------------------------------

	bool FXrControllerExtensions::HasTrackedHandAimPose() const
	{
		// The generic OpenXR aim pose is unusable for tracked hands (at the time of writing), so only the runtime's hand aim
		// (XR_FB_hand_tracking_aim) counts.
		return IsHandTrackingAvailable() && HandTrackers[0] != XR_NULL_HANDLE;
	}

	bool FXrControllerExtensions::GetTrackedHandAimPose(EControllerHand hand, FTransform& outWorldTransform) const
	{
		const int32 handIndex = (hand == EControllerHand::Left) ? 0 : 1;
		const IXRTrackingSystem* trackingSystem = GEngine ? GEngine->XRSystem.Get() : nullptr;
		if (trackingSystem == nullptr || !bIsAimValid[handIndex])
		{
			return false;
		}
		outWorldTransform = ToFTransform(AimPoses[handIndex], trackingSystem->GetWorldToMetersScale()) * trackingSystem->GetTrackingToWorldTransform();
		return true;
	}

	bool FXrControllerExtensions::IsHandTrackingActive(EControllerHand hand) const
	{
		return (hand == EControllerHand::AnyHand) ? ControllerMonitor.IsAnyHandTracked() : (ControllerMonitor.GetState(hand).Mode == EDrsXrControllerMode::TrackedHand);
	}

	bool FXrControllerExtensions::IsTrackedHandJointTracked(EControllerHand hand) const
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

	bool FXrControllerExtensions::IsPhysicalControllerGripTracked(EControllerHand hand) const
	{
		const IMotionController* motionController = GetMotionController();
		return motionController && motionController->GetControllerTrackingStatus(0, UDrsXrPlatformLibrary::GetGripSource(hand)) == ETrackingStatus::Tracked;
	}

	TOptional<bool> FXrControllerExtensions::DetermineHandTrackingAvailability() const
	{
		if (UDrsXrPlatformSettings::GetHandTrackingSupport() == EDrsXrHandTrackingSupport::ControllersOnly)
		{
			DRS_LOG(LogDrsXrPlatform, Log, "Hand tracking not available: the project supports controllers only (XR Platform settings).");
			return false;
		}

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

		DRS_LOG(LogDrsXrPlatform, Log, "Hand tracking available.");
		return true;
	}

	TOptional<bool> FXrControllerExtensions::IsHandTrackingSupportedForDevice() const
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

	const IHandTracker* FXrControllerExtensions::GetHandTracker() const
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

	const IMotionController* FXrControllerExtensions::GetMotionController() const
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

	void FXrControllerExtensions::DestroyHandTrackers()
	{
		for (int32 handIndex = 0; handIndex < 2; ++handIndex)
		{
			if (HandTrackers[handIndex] != XR_NULL_HANDLE)
			{
				DestroyHandTracker(HandTrackers[handIndex]);
				HandTrackers[handIndex] = XR_NULL_HANDLE;
			}
			bIsAimValid[handIndex] = false;
		}
	}
}
