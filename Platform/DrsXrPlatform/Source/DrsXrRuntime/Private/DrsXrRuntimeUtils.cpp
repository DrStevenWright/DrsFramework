// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrRuntimeUtils.h"

#include "DrsXrRuntime/DrsXrRuntime.h"

#include "DrsBase/DrsLog.h"

#include "HAL/PlatformMisc.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include <tlhelp32.h>
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

		bool IsProcessRunning(const TCHAR* processName)
		{
			const HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
			{
				return false;
			}

			PROCESSENTRY32W entry;
			entry.dwSize = sizeof(entry);
			bool isRunning = false;
			for (bool hasEntry = ::Process32FirstW(snapshot, &entry) != 0; hasEntry && !isRunning; hasEntry = ::Process32NextW(snapshot, &entry) != 0)
			{
				isRunning = FCString::Stricmp(entry.szExeFile, processName) == 0;
			}
			::CloseHandle(snapshot);
			return isRunning;
		}

		/** The default value of a URI protocol's open command, from the current user's classes or the machine's. */
		FString GetUriProtocolCommand(const TCHAR* protocol)
		{
			FString command;
			const FString userKey = FString::Printf(TEXT("SOFTWARE\\Classes\\%s\\shell\\open\\command"), protocol);
			const FString machineKey = FString::Printf(TEXT("%s\\shell\\open\\command"), protocol);
			if (!FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, *userKey, TEXT(""), command))
			{
				FWindowsPlatformMisc::QueryRegKey(HKEY_CLASSES_ROOT, *machineKey, TEXT(""), command);
			}
			return command;
		}
#else
		bool IsProcessRunning(const TCHAR*) { return false; }
#endif

		/** The first quoted path in a shell command, with forward slashes. */
		FString GetQuotedPath(const FString& command)
		{
			const FRegexPattern pattern(TEXT("\"(.+?)\""));
			FRegexMatcher matcher(pattern, command);
			return matcher.FindNext() ? matcher.GetCaptureGroup(1).Replace(TEXT("\\"), TEXT("/")) : FString();
		}

		FString ExistingFile(FString path)
		{
			if (!path.IsEmpty() && FPaths::CollapseRelativeDirectories(path))
			{
				DRS_LOG(LogDrsXrRuntime, Verbose, "Looking for a runtime manifest at '%s'.", *path);
				if (FPaths::FileExists(path))
				{
					FPaths::MakePlatformFilename(path);
					return path;
				}
			}
			return FString();
		}

		FString FindOculusRuntimeJson()
		{
#if PLATFORM_WINDOWS
			// The Meta Horizon Link installer sets OculusBase, e.g. "C:\Program Files\Meta Horizon\".
			const FString oculusBase = FPlatformMisc::GetEnvironmentVariable(TEXT("OculusBase"));
			if (!oculusBase.IsEmpty())
			{
				const FString candidate = ExistingFile(FPaths::ConvertRelativePathToFull(FPaths::Combine(oculusBase, TEXT("Support/oculus-runtime"), OculusManifestName)));
				if (!candidate.IsEmpty())
				{
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

		FString FindSteamVrRuntimeJson()
		{
#if PLATFORM_WINDOWS
			// e.g. "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrurlhandler.exe" "%1"
			const FString handler = GetQuotedPath(GetUriProtocolCommand(TEXT("steamvr")));
			if (!handler.IsEmpty())
			{
				return ExistingFile(FPaths::Combine(FPaths::GetPath(handler), TEXT("../../steamxr_win64.json")));
			}
#endif
			return FString();
		}

		FString FindPicoRuntimeJson(const TCHAR* clientFolder)
		{
#if PLATFORM_WINDOWS
			// e.g. "C:\Program Files\PICO Connect\openvr_driver\resources\ps_xrt\picostreaming-openxr.json"
			const FString programFiles = FPlatformMisc::GetEnvironmentVariable(TEXT("ProgramFiles"));
			if (!programFiles.IsEmpty())
			{
				return ExistingFile(FPaths::Combine(programFiles.Replace(TEXT("\\"), TEXT("/")), clientFolder, TEXT("openvr_driver/resources/ps_xrt/picostreaming-openxr.json")));
			}
#endif
			return FString();
		}
	}

	FRuntimeCandidate OculusRuntime()
	{
		return { EDrsXrRuntimeType::Oculus, 0, &FindOculusRuntimeJson, [] { return IsProcessRunning(TEXT("oculus-platform-runtime.exe")); } };
	}

	FRuntimeCandidate SteamVrRuntime()
	{
		return { EDrsXrRuntimeType::SteamVR, Drs::XrArbiter::ToRuntimeFlag(EDrsXrPlatformProperty::NonVrNotSupported), &FindSteamVrRuntimeJson,
			[] { return IsProcessRunning(TEXT("vrserver.exe")) || IsProcessRunning(TEXT("vrmonitor.exe")); } };
	}

	FRuntimeCandidate PicoStreamingRuntime()
	{
		return { EDrsXrRuntimeType::Pico, Drs::XrArbiter::ToRuntimeFlag(EDrsXrPlatformProperty::ControllerLosesFocus),
			[] { return FindPicoRuntimeJson(TEXT("Business Streaming")); }, [] { return IsProcessRunning(TEXT("Business Streaming.exe")); } };
	}

	FRuntimeCandidate PicoConnectRuntime()
	{
		return { EDrsXrRuntimeType::Pico, Drs::XrArbiter::ToRuntimeFlag(EDrsXrPlatformProperty::ControllerLosesFocus),
			[] { return FindPicoRuntimeJson(TEXT("PICO Connect")); }, [] { return IsProcessRunning(TEXT("PICO Connect.exe")); } };
	}

	bool IsPicoClientRunning()
	{
		return PicoStreamingRuntime().IsRunning() || PicoConnectRuntime().IsRunning();
	}

	void SetRuntimeJson(const FString& runtimeJson)
	{
		FPlatformMisc::SetEnvironmentVar(RuntimeJsonVariable, *runtimeJson);
	}
}
