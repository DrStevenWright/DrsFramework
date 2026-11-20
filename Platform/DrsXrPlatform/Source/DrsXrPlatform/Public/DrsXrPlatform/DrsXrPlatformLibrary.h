// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsBase/DrsHand.h"
#include "DrsXrPlatform/DrsXrControllerType.h"
#include "HeadMountedDisplayTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DrsXrPlatformLibrary.generated.h"

class UMotionControllerComponent;

/** Cross-vendor foveated rendering levels. */
UENUM(BlueprintType)
enum class EDrsXrFoveationLevel : uint8
{
	None = 0,
	Low,
	Medium,
	High,
};

/**
 * The XR platform API for higher layers: XR modes, HMD state, motion controller and hand-tracking queries, and
 * rendering options. Prefer these to the engine's HeadMountedDisplayFunctionLibrary, so that vendor and runtime details
 * stay in this plugin. The state lives in FDrsXrPlatformModule. Local-only: holds no replicated state.
 */
UCLASS()
class DRSXRPLATFORM_API UDrsXrPlatformLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//------------------------------------------------------------------
	// XR system and modes
	//------------------------------------------------------------------

	/** True when the engine has an active XR tracking system (e.g. OpenXR). */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform")
	static bool IsXrSystemActive();

	/** Name of the active XR tracking system, or None. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform")
	static FName GetXrSystemName();

	/** Whether the app runs without VR (-noHMD, the editor without VR Preview, or no HMD connected). */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform", meta = (BlueprintThreadSafe))
	static bool IsNonVrMode();

	/** The vendor XR plugin in use, such as Meta or Pico, or None for plain OpenXR. Compare it with a vendor's ID. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform")
	static FName GetXrPluginType();

	/** The OpenXR runtime in use, such as SteamVR or Oculus, or Android on standalone devices. None if DRS selected none. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform")
	static FName GetXrRuntimeType();

	/** Whether the app has XR focus: the HMD is enabled and worn, and the system menu is closed. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform")
	static bool HasXrFocus();

	/** Whether the headset's system menu is open. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform")
	static bool IsSystemMenuOpen();

	//------------------------------------------------------------------
	// HMD
	//------------------------------------------------------------------

	/** The connected HMD's device name. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static FName GetHmdDeviceName();

	/**
	 * The active device profile. On Android the device profile rules select it (or -dp= overrides it); elsewhere it is
	 * the platform default (e.g. "Windows", or "WindowsEditor" in the editor), whatever the HMD.
	 */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static FString GetDeviceProfileName();

	/**
	 * Whether the named device profile is active, or is a parent of the active one. Profiles describe performance tiers,
	 * so this also matches every device whose profile inherits it.
	 */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static bool IsDeviceProfileActive(const FString& profileName);

	/** Whether an HMD is connected (it may not be in use). */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static bool IsHmdConnected();

	/** Whether an HMD is enabled (in use). */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static bool IsHmdEnabled();

	/** Whether an HMD is enabled and tracking. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static bool IsHmdTracking();

	/** Whether the HMD is worn. On devices that can't sense it, this is a best guess from whether the app is in use. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static bool IsHmdWorn();

	/** The HMD worn state (see IsHmdWorn). */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|HMD")
	static EHMDWornState::Type GetHmdWornState();

	/** Enables or disables the HMD (stereo rendering). */
	UFUNCTION(BlueprintCallable, Category = "DRS|XR Platform|HMD")
	static void EnableHmd(bool enable);

	/** Enables the HMD and stereo rendering where VR is in use, and sets the floor tracking origin. Runs once (every PIE session in the editor). */
	static void SetupHmd();

	/**
	 * Puts the tracking-space origin on the floor, centred where the user started. Devices that can't find the floor keep
	 * the origin at head height. Returns false when no XR system is active.
	 */
	UFUNCTION(BlueprintCallable, Category = "DRS|XR Platform|HMD")
	static bool SetFloorTrackingOrigin();

	/** Whether the floor tracking origin has been requested, or is already set. */
	static bool IsTrackingOriginSetup();

	/** The HMD's tracking-space pose and the world-to-metres scale. */
	static void GetHmdDevicePose(UObject* worldContext, FRotator& outRotation, FVector& outPosition, float& outWorldToMetresScale);

	//------------------------------------------------------------------
	// Motion controllers
	//------------------------------------------------------------------

	/** Both hands' controllers and their modes. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Controllers")
	static FDrsXrControllerType GetControllerType();

	/** Whether every hand in the mask has an active controller: a physical device or a tracked hand. False for None. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Controllers")
	static bool IsControllerPresent(EDrsHandMask hands);

	/** The world-space grip pose of a hand. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Controllers")
	static bool GetGripPose(EDrsHand hand, FTransform& outWorldTransform);

	/** The world-space aim pose of a hand: the pointing ray of its controller, or of the tracked hand. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Controllers")
	static bool GetAimPose(EDrsHand hand, FTransform& outWorldTransform);

	/** The world-space palm pose: OpenXR grip_surface (1.1) or palm_ext (XR_EXT_palm_pose), or the grip pose without either. */
	static bool GetPalmPose(EDrsHand hand, FTransform& outWorldTransform);

	/** The engine's OpenXR motion sources for a hand's grip, aim and palm poses. */
	static FName GetGripSource(EDrsHand hand);
	static FName GetAimSource(EDrsHand hand);
	static FName GetPalmSource(EDrsHand hand);

	/** The world-space pose of a motion source, as reported by one motion controller device (several can share a source name). */
	static bool GetMotionControllerPose(FName deviceName, FName source, FTransform& outWorldTransform);

	/** The motion source for a hand: the palm pose for physical controllers, the wrist joint for tracked hands. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Controllers")
	static FName GetMotionControllerSource(EDrsHand hand, bool isHandTracking = false);

	/** A motion controller's tracking status (for a tracked hand, whether the hand is tracked). */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Controllers")
	static ETrackingStatus GetMotionControllerTrackingStatus(const UMotionControllerComponent* controller);

	/** Whether a physical motion controller is driving the input of every hand in the mask. False for None. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Controllers")
	static bool IsPhysicalControllerActive(EDrsHandMask hands);

	//------------------------------------------------------------------
	// Hand tracking
	//------------------------------------------------------------------

	/** Whether hand tracking is possible with this device, runtime and project setup. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Hand Tracking")
	static bool IsHandTrackingAvailable();

	/** Whether every hand in the mask is in tracked-hand mode. False for None. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Hand Tracking")
	static bool IsHandTrackingActive(EDrsHandMask hands);

	/** Whether the device gives a dedicated pointing ray for tracked hands. Without one, Get Aim Pose may be less stable for tracked hands. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Hand Tracking")
	static bool HasTrackedHandAimPose();

	/** The world-space joints of a tracked hand, if available. While a hand is not tracked, its last pose. */
	UFUNCTION(BlueprintCallable, Category = "DRS|XR Platform|Hand Tracking")
	static bool GetHandTrackingData(EDrsHand hand, TArray<FVector>& outPositions, TArray<FQuat>& outRotations, TArray<float>& outRadii);

	//------------------------------------------------------------------
	// Rendering
	//------------------------------------------------------------------

	/** Sets the foveated rendering level, on every vendor. */
	UFUNCTION(BlueprintCallable, Category = "DRS|XR Platform|Rendering")
	static void SetFoveationLevel(EDrsXrFoveationLevel foveationLevel, bool isDynamic);

	/** Requests a display refresh rate (Hz). An unsupported rate is replaced by the closest supported one. */
	UFUNCTION(BlueprintCallable, Category = "DRS|XR Platform|Rendering")
	static void SetDisplayFrequency(float frequency);

	/** The display refresh rate (Hz), or 0 if unknown. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Rendering")
	static float GetDisplayFrequency();

	/** GPU utilisation in percent (0-100). Returns false where the device doesn't report it. */
	UFUNCTION(BlueprintCallable, Category = "DRS|XR Platform|Rendering")
	static bool GetGpuUtilisation(float& outGpuUtilisation);

	/** Sets the spectator screen mode, used for mirroring. */
	UFUNCTION(BlueprintCallable, Category = "DRS|XR Platform|Rendering")
	static void SetSpectatorScreenMode(ESpectatorScreenMode screenMode);

	/** The spectator screen mode. */
	UFUNCTION(BlueprintPure, Category = "DRS|XR Platform|Rendering")
	static ESpectatorScreenMode GetSpectatorScreenMode();
};
