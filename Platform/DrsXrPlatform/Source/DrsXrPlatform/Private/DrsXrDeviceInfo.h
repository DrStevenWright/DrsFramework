// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsXrControllerMonitor.h"
#include "IOpenXRExtensionPlugin.h"
#include "Tickable.h"
#include <openxr/openxr.h>

class IDrsXrHandTrackingProvider;
class IHandTracker;
class IModularFeature;
class IMotionController;

/**
 * Tracks the XR device state through the native OpenXR API: HMD worn state, system menu, focus, controller modes,
 * hand-tracking availability and display refresh rate. Registered as an OpenXR extension plugin, so it is created at
 * PostConfigInit, before OpenXR creates its instance. Local-only: holds no replicated state.
 */
class FDrsXrDeviceInfo : public IOpenXRExtensionPlugin, public FTickableGameObject, public IDrsXrControllerSource
{
public:
	FDrsXrDeviceInfo();
	virtual ~FDrsXrDeviceInfo() override;

	bool HasFocus() const { return bHasFocus; }
	bool IsHmdWorn() const { return bIsHmdWorn; }
	bool IsSystemMenuOpen() const { return bIsSystemMenuOpen; }
	bool IsHandTrackingAvailable() const;
	const FDrsXrControllerType& GetControllerType();

	bool IsControllerPresent(EControllerHand hand) const { return ControllerMonitor.IsControllerPresent(hand); }
	bool IsPhysicalControllerActive(EControllerHand hand) const { return ControllerMonitor.IsPhysicalControllerActive(hand); }
	bool IsHandTrackingActive(EControllerHand hand) const;
	bool HasTrackedHandAimPose() const;
	bool GetTrackedHandAimPose(EControllerHand hand, FTransform& outWorldTransform) const;
	float GetDisplayFrequency() const;
	bool SetDisplayFrequency(float frequency);

	// FTickableGameObject
	virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FDrsXrDeviceInfo, STATGROUP_Tickables); }
	virtual bool IsAllowedToTick() const override { return !UE_SERVER; }

	// IOpenXRExtensionPlugin
	virtual FString GetDisplayName() override { return TEXT("DrsXrDeviceInfo"); }
	virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& outExtensions) override;
	virtual void OnEvent(XrSession inSession, const XrEventDataBaseHeader* inHeader) override;
	virtual void PostCreateInstance(XrInstance inInstance) override;
	virtual const void* OnCreateSession(XrInstance inInstance, XrSystemId inSystem, const void* inNext) override;
	virtual void PostCreateSession(XrSession inSession) override;
	virtual void OnDestroySession(XrSession inSession) override;
	virtual void UpdateDeviceLocations(XrSession inSession, XrTime displayTime, XrSpace trackingSpace) override;

	// IDrsXrControllerSource
	virtual bool CanTrackHands() const override { return IsHandTrackingAvailable(); }
	virtual bool AreMixedControllerModesEnabled() const override { return MixedControllerModesEnabled.Get(false); }
	virtual bool IsTrackedHandJointTracked(EControllerHand hand) const override;
	virtual bool IsPhysicalControllerGripTracked(EControllerHand hand) const override;

private:
	// Device state updates. Each broadcasts the matching FDrsXrDelegates event when the state changes.
	void SetHmdWorn(bool isWorn);
	void SetSystemMenuOpen(bool isOpen);
	void SetControllerStates(const FDrsXrControllerState& left, const FDrsXrControllerState& right);
	void UpdateFocus();

	TOptional<bool> DetermineHandTrackingAvailability() const;
	TOptional<bool> IsHandTrackingSupportedForDevice() const; // Unset until the OpenXR session exists.
	void EnableMixedControllerModes();
	const IHandTracker* GetHandTracker() const;
	const IMotionController* GetMotionController() const;
	void OnModularFeatureRegistered(const FName& type, IModularFeature* modularFeature);
	void OnModularFeatureUnregistered(const FName& type, IModularFeature* modularFeature);

	bool bHasFocus = false;
	bool bIsHmdWorn = false;
	bool bIsSystemMenuOpen = false;
	FDrsXrControllerType ControllerType;
	mutable TOptional<bool> HandTrackingAvailable; // Unset until known for the current session.

	FDrsXrControllerMonitor ControllerMonitor{ *this };
	IDrsXrHandTrackingProvider* HandTrackingProvider = nullptr;
	TOptional<bool> MixedControllerModesEnabled; // Unset until decided for the session (also after a failed attempt, which is retried).
	int32 SystemMenuFrameCount = 0;

	// Cached for the per-frame joint queries.
	mutable const IHandTracker* CachedHandTracker = nullptr;
	mutable const IMotionController* CachedMotionController = nullptr;
	mutable TArray<FVector> JointPositions;
	mutable TArray<FQuat> JointRotations;
	mutable TArray<float> JointRadii;

	XrSession Session = XR_NULL_HANDLE;
	XrSessionState SessionState = XR_SESSION_STATE_UNKNOWN;

	// HMD worn state (XR_EXT_user_presence). Until the runtime reports user presence, it is inferred from the session state.
	bool bUserPresenceSupported = false;
	bool bUserPresenceReported = false;

	// Display refresh rate (XR_FB_display_refresh_rate).
	bool bDisplayRefreshRateSupported = false;
	float CurrentDisplayRefreshRate = 0.0f;
	TArray<float> SupportedDisplayRefreshRates;
	PFN_xrEnumerateDisplayRefreshRatesFB EnumerateDisplayRefreshRates = nullptr;
	PFN_xrGetDisplayRefreshRateFB GetDisplayRefreshRate = nullptr;
	PFN_xrRequestDisplayRefreshRateFB RequestDisplayRefreshRate = nullptr;
};
