// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrControllerExtensions.h"
#include "IOpenXRExtensionPlugin.h"
#include <openxr/openxr.h>

namespace Drs::XrPlatform
{
	/**
	 * The XR device state, tracked through OpenXR: HMD worn state, system menu, focus, display refresh rate and GPU
	 * utilisation. An OpenXR extension plugin, so it is created at PostConfigInit, before OpenXR creates its instance. It
	 * owns FXrControllerExtensions, a second extension plugin for the hands and controllers. UDrsXrPlatformLibrary is the
	 * way in. Local-only: holds no replicated state.
	 */
	class FXrDeviceExtensions : public IOpenXRExtensionPlugin
	{
	public:
		FXrDeviceExtensions();
		virtual ~FXrDeviceExtensions() override;

		bool HasFocus() const { return bHasFocus; }
		bool IsHmdWorn() const { return bIsHmdWorn; }
		bool IsSystemMenuOpen() const { return bIsSystemMenuOpen; }
		float GetDisplayFrequency() const;
		bool SetDisplayFrequency(float frequency);
		bool GetGpuUtilisation(float& outPercent) const;

		/** The hands and controllers. */
		FXrControllerExtensions& GetControllerExtensions() { return ControllerExtensions; }

		// IOpenXRExtensionPlugin
		virtual FString GetDisplayName() override { return TEXT("DrsXrDeviceExtensions"); }
		virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& outExtensions) override;
		virtual void OnEvent(XrSession inSession, const XrEventDataBaseHeader* inHeader) override;
		virtual void PostCreateInstance(XrInstance inInstance) override;
		virtual const void* OnCreateSession(XrInstance inInstance, XrSystemId inSystem, const void* inNext) override;
		virtual void PostCreateSession(XrSession inSession) override;
		virtual void OnDestroySession(XrSession inSession) override;
		virtual void UpdateDeviceLocations(XrSession inSession, XrTime displayTime, XrSpace trackingSpace) override;

	private:
		// Device state updates. Each broadcasts the matching FXrDelegates event when the state changes.
		void SetHmdWorn(bool isWorn);
		void SetSystemMenuOpen(bool isOpen);
		void UpdateFocus();

		void InitialisePerformanceMetrics();

		FXrControllerExtensions ControllerExtensions; // Its own extension plugin.

		bool bHasFocus = false;
		bool bIsHmdWorn = false;
		bool bIsSystemMenuOpen = false;
		int32 SystemMenuFrameCount = 0;

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

		// GPU utilisation (XR_META_performance_metrics). Only standalone Quest reports it; Link and SteamVR give frame times only.
		XrInstance Instance = XR_NULL_HANDLE;
		bool bPerformanceMetricsSupported = false;
		XrPath GpuUtilisationPath = XR_NULL_PATH; // Null when the session doesn't report GPU utilisation.
		PFN_xrEnumeratePerformanceMetricsCounterPathsMETA EnumeratePerformanceMetricsCounterPaths = nullptr;
		PFN_xrSetPerformanceMetricsStateMETA SetPerformanceMetricsState = nullptr;
		PFN_xrQueryPerformanceMetricsCounterMETA QueryPerformanceMetricsCounter = nullptr;
	};
}
