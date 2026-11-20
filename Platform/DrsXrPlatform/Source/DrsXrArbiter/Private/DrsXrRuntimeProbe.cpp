// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrRuntimeProbe.h"

#include "DrsBase/DrsLog.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#if PLATFORM_ANDROID
#include "Android/AndroidJavaEnv.h"
#include <jni.h>
#define XR_USE_PLATFORM_ANDROID 1
#endif

#include <openxr/openxr.h>

namespace Drs::XrArbiter
{
	namespace
	{
		template <typename FunctionType>
		FunctionType GetFunction(PFN_xrGetInstanceProcAddr getProc, XrInstance instance, const char* name)
		{
			FunctionType function = nullptr;
			getProc(instance, name, reinterpret_cast<PFN_xrVoidFunction*>(&function));
			return function;
		}

		/** Loads the OpenXR loader that OpenXRHMD also uses, and returns its entry point or null. */
		PFN_xrGetInstanceProcAddr LoadLoader()
		{
#if PLATFORM_WINDOWS
			const FString binariesPath = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/OpenXR/win64");
			FPlatformProcess::PushDllDirectory(*binariesPath);
			void* loaderHandle = FPlatformProcess::GetDllHandle(TEXT("openxr_loader.dll"));
			FPlatformProcess::PopDllDirectory(*binariesPath);
#elif PLATFORM_ANDROID
			void* loaderHandle = FPlatformProcess::GetDllHandle(TEXT("libopenxr_loader.so"));
#else
			void* loaderHandle = nullptr;
#endif
			if (loaderHandle == nullptr)
			{
				DRS_LOG(LogDrsXrArbiter, Warning, "Runtime probe: couldn't load the OpenXR loader.");
				return nullptr;
			}
			
			const PFN_xrGetInstanceProcAddr getProc = reinterpret_cast<PFN_xrGetInstanceProcAddr>(FPlatformProcess::GetDllExport(loaderHandle, TEXT("xrGetInstanceProcAddr")));
			if (getProc == nullptr)
			{
				DRS_LOG(LogDrsXrArbiter, Warning, "Runtime probe: the OpenXR loader has no xrGetInstanceProcAddr.");
				return nullptr;
			}

#if PLATFORM_ANDROID
			// OpenXRHMD initialises the loader again later, which is harmless (checked on Quest 3).
			const PFN_xrInitializeLoaderKHR initializeLoader = GetFunction<PFN_xrInitializeLoaderKHR>(getProc, XR_NULL_HANDLE, "xrInitializeLoaderKHR");
			JavaVM* vm = nullptr;
			AndroidJavaEnv::GetJavaEnv()->GetJavaVM(&vm);
			XrLoaderInitInfoAndroidKHR loaderInfo{ XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR };
			loaderInfo.applicationVM = vm;
			loaderInfo.applicationContext = AndroidJavaEnv::GetGameActivityThis();
			if (initializeLoader == nullptr || XR_FAILED(initializeLoader(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loaderInfo))))
			{
				DRS_LOG(LogDrsXrArbiter, Warning, "Runtime probe: couldn't initialise the OpenXR loader.");
				return nullptr;
			}
#endif
			return getProc;
		}

		TOptional<FXrRuntimeProbe> RunProbe()
		{
			const PFN_xrGetInstanceProcAddr getProc = LoadLoader();
			if (getProc == nullptr)
			{
				return {};
			}

			XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
			FCStringAnsi::Strncpy(createInfo.applicationInfo.applicationName, "DrsXrArbiter", XR_MAX_APPLICATION_NAME_SIZE);
			FCStringAnsi::Strncpy(createInfo.applicationInfo.engineName, "UnrealEngine", XR_MAX_ENGINE_NAME_SIZE);
			createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
#if PLATFORM_ANDROID
			const char* const extensions[] = { XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME };
			createInfo.enabledExtensionCount = UE_ARRAY_COUNT(extensions);
			createInfo.enabledExtensionNames = extensions;

			JavaVM* vm = nullptr;
			AndroidJavaEnv::GetJavaEnv()->GetJavaVM(&vm);
			XrInstanceCreateInfoAndroidKHR androidInfo{ XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR };
			androidInfo.applicationVM = vm;
			androidInfo.applicationActivity = AndroidJavaEnv::GetGameActivityThis();
			createInfo.next = &androidInfo;
#endif

			const PFN_xrCreateInstance createInstance = GetFunction<PFN_xrCreateInstance>(getProc, XR_NULL_HANDLE, "xrCreateInstance");
			XrInstance instance = XR_NULL_HANDLE;
			const double startTime = FPlatformTime::Seconds();
			const XrResult createResult = createInstance != nullptr ? createInstance(&createInfo, &instance) : XR_ERROR_FUNCTION_UNSUPPORTED;
			if (XR_FAILED(createResult))
			{
				DRS_LOG(LogDrsXrArbiter, Log, "Runtime probe: the runtime refused an instance (%d), which means no headset.", static_cast<int32>(createResult));
				return {};
			}

			FXrRuntimeProbe probe;
			XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
			const PFN_xrGetInstanceProperties getInstanceProperties = GetFunction<PFN_xrGetInstanceProperties>(getProc, instance, "xrGetInstanceProperties");
			if (getInstanceProperties != nullptr && XR_SUCCEEDED(getInstanceProperties(instance, &instanceProperties)))
			{
				probe.RuntimeName = UTF8_TO_TCHAR(instanceProperties.runtimeName);
			}

			// xrGetSystem fails with no headset connected (XR_ERROR_FORM_FACTOR_UNAVAILABLE); the runtime name still counts.
			XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
			systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
			XrSystemId systemId = XR_NULL_SYSTEM_ID;
			const PFN_xrGetSystem getSystem = GetFunction<PFN_xrGetSystem>(getProc, instance, "xrGetSystem");
			const PFN_xrGetSystemProperties getSystemProperties = GetFunction<PFN_xrGetSystemProperties>(getProc, instance, "xrGetSystemProperties");
			XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
			if (getSystem != nullptr && getSystemProperties != nullptr && XR_SUCCEEDED(getSystem(instance, &systemInfo, &systemId))
				&& XR_SUCCEEDED(getSystemProperties(instance, systemId, &systemProperties)))
			{
				probe.SystemName = UTF8_TO_TCHAR(systemProperties.systemName);
				probe.VendorId = systemProperties.vendorId;
			}

			if (const PFN_xrDestroyInstance destroyInstance = GetFunction<PFN_xrDestroyInstance>(getProc, instance, "xrDestroyInstance"))
			{
				destroyInstance(instance);
			}

			// The vendor ID is logged only: Meta's runtime reports a different one over Link.
			DRS_LOG(LogDrsXrArbiter, Log, "Runtime probe: runtime '%s', system '%s', vendor ID 0x%x, in %.0f ms.",
				*probe.RuntimeName, *probe.SystemName, probe.VendorId, (FPlatformTime::Seconds() - startTime) * 1000.0);
			return probe;
		}
	}

	const TOptional<FXrRuntimeProbe>& ProbeXrRuntime()
	{
		static const TOptional<FXrRuntimeProbe> probe = RunProbe();
		return probe;
	}
}
