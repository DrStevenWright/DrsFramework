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
			TArray<FString> Result;
			HKEY Key;
			if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes", 0, KEY_READ, &Key) != ERROR_SUCCESS)
			{
				return Result;
			}
			for (DWORD Index = 0;; ++Index)
			{
				WCHAR Name[MAX_PATH * 2];
				DWORD NameLength = UE_ARRAY_COUNT(Name);
				if (::RegEnumValueW(Key, Index, Name, &NameLength, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
				{
					break;
				}
				Result.Emplace(Name);
			}
			::RegCloseKey(Key);
			return Result;
		}
#endif
	}

	FString FindOculusRuntimeJson()
	{
#if PLATFORM_WINDOWS
		// The Meta Horizon Link installer sets OculusBase, e.g. "C:\Program Files\Meta Horizon\".
		const FString OculusBase = FPlatformMisc::GetEnvironmentVariable(TEXT("OculusBase"));
		if (!OculusBase.IsEmpty())
		{
			FString Candidate = FPaths::ConvertRelativePathToFull(FPaths::Combine(OculusBase, TEXT("Support/oculus-runtime"), OculusManifestName));
			UE_LOG(LogDrsXrRuntime, Verbose, TEXT("Looking for the Oculus runtime at '%s'."), *Candidate);
			if (FPaths::FileExists(Candidate))
			{
				FPaths::MakePlatformFilename(Candidate);
				return Candidate;
			}
		}

		for (const FString& Registered : GetRegisteredRuntimeJsons())
		{
			if (FPaths::GetCleanFilename(Registered).Equals(OculusManifestName, ESearchCase::IgnoreCase) && FPaths::FileExists(Registered))
			{
				return Registered;
			}
		}
#endif
		return FString();
	}

	void SetRuntimeJson(const FString& RuntimeJson)
	{
		FPlatformMisc::SetEnvironmentVar(RuntimeJsonVariable, *RuntimeJson);
	}
}
