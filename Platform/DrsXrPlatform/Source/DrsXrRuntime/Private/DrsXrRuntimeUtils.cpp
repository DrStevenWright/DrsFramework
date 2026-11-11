// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrRuntimeUtils.h"
#include "DrsXrRuntime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace Drs::XrRuntime
{
	namespace
	{
		const TCHAR* OculusManifestName = TEXT("oculus_openxr_64.json");

#if PLATFORM_WINDOWS
		/** Runtime manifests registered with the OpenXR loader. Each value name is a manifest path. */
		TArray<FString> GetRegisteredRuntimeJsons()
		{
			TArray<FString> result;
			HKEY key;
			if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes", 0, KEY_READ, &key) != ERROR_SUCCESS)
			{
				return result;
			}
			for (DWORD index = 0;; ++index)
			{
				WCHAR name[MAX_PATH * 2];
				DWORD nameLength = UE_ARRAY_COUNT(name);
				if (::RegEnumValueW(key, index, name, &nameLength, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
				{
					break;
				}
				result.Emplace(name);
			}
			::RegCloseKey(key);
			return result;
		}
#endif
	}

	FString FindOculusRuntimeJson()
	{
#if PLATFORM_WINDOWS
		// The Meta Horizon Link installer sets OculusBase, e.g. "C:\Program Files\Meta Horizon\".
		const FString oculusBase = FPlatformMisc::GetEnvironmentVariable(TEXT("OculusBase"));
		if (!oculusBase.IsEmpty())
		{
			FString candidate = FPaths::ConvertRelativePathToFull(FPaths::Combine(oculusBase, TEXT("Support/oculus-runtime"), OculusManifestName));
			UE_LOG(LogDrsXrRuntime, Verbose, TEXT("Looking for the Oculus runtime at '%s'."), *candidate);
			if (FPaths::FileExists(candidate))
			{
				FPaths::MakePlatformFilename(candidate);
				return candidate;
			}
		}

		for (const FString& registered : GetRegisteredRuntimeJsons())
		{
			if (FPaths::GetCleanFilename(registered).Equals(OculusManifestName, ESearchCase::IgnoreCase) && FPaths::FileExists(registered))
			{
				return registered;
			}
		}
#endif
		return FString();
	}

	void SetRuntimeJson(const FString& runtimeJson)
	{
		FPlatformMisc::SetEnvironmentVar(RuntimeJsonVariable, *runtimeJson);
	}
}
