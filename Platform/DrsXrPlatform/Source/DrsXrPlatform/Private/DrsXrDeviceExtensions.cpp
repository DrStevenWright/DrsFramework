// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrDeviceExtensions.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrPlatform/DrsXrDelegates.h"
#include "DrsXrPlatform/DrsXrPlatform.h"
#include "Features/IModularFeatures.h"
#include "IOpenXRHMDModule.h"
#include "OpenXRCore.h"

namespace Drs::XrPlatform
{
	FXrDeviceExtensions::FXrDeviceExtensions()
	{
		if (!Drs::XrArbiter::IsNonVrMode())
		{
			RegisterOpenXRExtensionModularFeature();
		}
	}

	FXrDeviceExtensions::~FXrDeviceExtensions()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	//------------------------------------------------------------------
	// State
	//------------------------------------------------------------------

	void FXrDeviceExtensions::SetHmdWorn(bool isWorn)
	{
		if (isWorn != bIsHmdWorn)
		{
			DRS_LOG(LogDrsXrPlatform, Log, "HMD worn: %d.", isWorn);
			bIsHmdWorn = isWorn;
			FXrDelegates::OnHmdWornStateChanged.Broadcast(isWorn);
			UpdateFocus();
		}
	}

	void FXrDeviceExtensions::SetSystemMenuOpen(bool isOpen)
	{
		if (isOpen != bIsSystemMenuOpen)
		{
			DRS_LOG(LogDrsXrPlatform, Log, "System menu open: %d.", isOpen);
			bIsSystemMenuOpen = isOpen;
			FXrDelegates::OnSystemMenuStateChanged.Broadcast(isOpen);
			UpdateFocus();
		}
	}

	void FXrDeviceExtensions::UpdateFocus()
	{
		const bool hasFocus = bIsHmdWorn && !bIsSystemMenuOpen;
		if (hasFocus != bHasFocus)
		{
			DRS_LOG(LogDrsXrPlatform, Log, "XR focus: %d.", hasFocus);
			bHasFocus = hasFocus;
			FXrDelegates::OnXrFocusStateChanged.Broadcast(hasFocus);
		}
	}

	//------------------------------------------------------------------
	// OpenXR callbacks
	//------------------------------------------------------------------

	bool FXrDeviceExtensions::GetOptionalExtensions(TArray<const ANSICHAR*>& outExtensions)
	{
		outExtensions.Add(XR_EXT_USER_PRESENCE_EXTENSION_NAME); // HMD worn state, where supported.
		outExtensions.Add(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME); // Display refresh rate control, where supported.
		outExtensions.Add(XR_META_PERFORMANCE_METRICS_EXTENSION_NAME); // GPU utilisation, where supported.
		return true;
	}

	void FXrDeviceExtensions::OnEvent(XrSession inSession, const XrEventDataBaseHeader* inHeader)
	{
		if (inHeader->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
		{
			const XrEventDataSessionStateChanged& sessionState = reinterpret_cast<const XrEventDataSessionStateChanged&>(*inHeader);
			DRS_LOG(LogDrsXrPlatform, Log, "OpenXR session state: %s.", OpenXRSessionStateToString(sessionState.state));
			SessionState = sessionState.state;

			switch (SessionState)
			{
			case XR_SESSION_STATE_FOCUSED:
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

	void FXrDeviceExtensions::PostCreateInstance(XrInstance inInstance)
	{
		bDisplayRefreshRateSupported = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME)
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrEnumerateDisplayRefreshRatesFB", reinterpret_cast<PFN_xrVoidFunction*>(&EnumerateDisplayRefreshRates)))
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrGetDisplayRefreshRateFB", reinterpret_cast<PFN_xrVoidFunction*>(&GetDisplayRefreshRate)))
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrRequestDisplayRefreshRateFB", reinterpret_cast<PFN_xrVoidFunction*>(&RequestDisplayRefreshRate)));
		DRS_LOG(LogDrsXrPlatform, Log, "OpenXR display refresh rate control supported: %s.", *LexToString(bDisplayRefreshRateSupported));

		Instance = inInstance;
		bPerformanceMetricsSupported = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_META_PERFORMANCE_METRICS_EXTENSION_NAME)
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrEnumeratePerformanceMetricsCounterPathsMETA", reinterpret_cast<PFN_xrVoidFunction*>(&EnumeratePerformanceMetricsCounterPaths)))
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrSetPerformanceMetricsStateMETA", reinterpret_cast<PFN_xrVoidFunction*>(&SetPerformanceMetricsState)))
			&& XR_ENSURE(xrGetInstanceProcAddr(inInstance, "xrQueryPerformanceMetricsCounterMETA", reinterpret_cast<PFN_xrVoidFunction*>(&QueryPerformanceMetricsCounter)));
	}

	const void* FXrDeviceExtensions::OnCreateSession(XrInstance inInstance, XrSystemId inSystem, const void* inNext)
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

	void FXrDeviceExtensions::PostCreateSession(XrSession inSession)
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

		InitialisePerformanceMetrics();
	}

	void FXrDeviceExtensions::InitialisePerformanceMetrics()
	{
		GpuUtilisationPath = XR_NULL_PATH;
		if (!bPerformanceMetricsSupported)
		{
			return;
		}

		// Enabling the metrics again is harmless, so this runs even when a vendor plugin (MetaXR) has already done it.
		XrPerformanceMetricsStateMETA state = { XR_TYPE_PERFORMANCE_METRICS_STATE_META };
		state.enabled = XR_TRUE;
		if (!XR_ENSURE(SetPerformanceMetricsState(Session, &state)))
		{
			return;
		}

		// The runtime lists the counters it reports. Over Link only frame times are listed.
		TArray<XrPath> counterPaths;
		uint32 count = 0;
		if (XR_ENSURE(EnumeratePerformanceMetricsCounterPaths(Instance, 0, &count, nullptr)))
		{
			counterPaths.SetNumZeroed(count);
			const bool hasPaths = XR_ENSURE(EnumeratePerformanceMetricsCounterPaths(Instance, count, &count, counterPaths.GetData()));
			counterPaths.SetNum(hasPaths ? count : 0);
		}

		XrPath gpuUtilisationPath = XR_NULL_PATH;
		if (XR_ENSURE(xrStringToPath(Instance, "/perfmetrics_meta/device/gpu_utilization", &gpuUtilisationPath)) && counterPaths.Contains(gpuUtilisationPath))
		{
			GpuUtilisationPath = gpuUtilisationPath;
		}
		DRS_LOG(LogDrsXrPlatform, Log, "OpenXR GPU utilisation (XR_META_performance_metrics) reported: %s.", *LexToString(GpuUtilisationPath != XR_NULL_PATH));
	}

	void FXrDeviceExtensions::OnDestroySession(XrSession inSession)
	{
		Session = XR_NULL_HANDLE;
		CurrentDisplayRefreshRate = 0.0f;
		SupportedDisplayRefreshRates.Reset();
		GpuUtilisationPath = XR_NULL_PATH;
	}

	void FXrDeviceExtensions::UpdateDeviceLocations(XrSession inSession, XrTime displayTime, XrSpace trackingSpace)
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

	//------------------------------------------------------------------
	// Display refresh rate
	//------------------------------------------------------------------

	float FXrDeviceExtensions::GetDisplayFrequency() const
	{
		check(IsInGameThread());
		return CurrentDisplayRefreshRate;
	}

	bool FXrDeviceExtensions::SetDisplayFrequency(float frequency)
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

	//------------------------------------------------------------------
	// GPU utilisation
	//------------------------------------------------------------------

	bool FXrDeviceExtensions::GetGpuUtilisation(float& outPercent) const
	{
		check(IsInGameThread());
		outPercent = 0.0f;
		if (Session == XR_NULL_HANDLE || GpuUtilisationPath == XR_NULL_PATH)
		{
			return false;
		}

		XrPerformanceMetricsCounterMETA counter = { XR_TYPE_PERFORMANCE_METRICS_COUNTER_META };
		if (XR_FAILED(QueryPerformanceMetricsCounter(Session, GpuUtilisationPath, &counter)) || !(counter.counterFlags & XR_PERFORMANCE_METRICS_COUNTER_FLOAT_VALUE_VALID_BIT_META))
		{
			return false;
		}
		outPercent = counter.floatValue;
		return true;
	}
}
