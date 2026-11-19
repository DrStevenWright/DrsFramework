// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrRuntimeSelection.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"
#include "HAL/PlatformMisc.h"
#include "Internationalization/Regex.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include <tlhelp32.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY(LogDrsXrRuntime);

namespace Drs::XrRuntime
{
	namespace
	{
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

		/** The file names of every running process, in lower case. */
		TSet<FString> GetRunningProcessNames()
		{
			TSet<FString> result;
			const HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
			{
				return result;
			}

			PROCESSENTRY32W entry;
			entry.dwSize = sizeof(entry);
			for (bool hasEntry = ::Process32FirstW(snapshot, &entry) != 0; hasEntry; hasEntry = ::Process32NextW(snapshot, &entry) != 0)
			{
				result.Add(FString(entry.szExeFile).ToLower());
			}
			::CloseHandle(snapshot);
			return result;
		}

		/** The default value of a URI protocol's open command, from the current user's classes or the machine's. */
		FString GetUriProtocolCommand(const FString& protocol)
		{
			FString command;
			const FString userKey = FString::Printf(TEXT("SOFTWARE\\Classes\\%s\\shell\\open\\command"), *protocol);
			const FString machineKey = FString::Printf(TEXT("%s\\shell\\open\\command"), *protocol);
			if (!FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, *userKey, TEXT(""), command))
			{
				FWindowsPlatformMisc::QueryRegKey(HKEY_CLASSES_ROOT, *machineKey, TEXT(""), command);
			}
			return command;
		}

		/** Steam's install folder, from the current user's registry or the machine's. */
		FString GetSteamFolder()
		{
			FString folder;
			if (!FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, TEXT("Software\\Valve\\Steam"), TEXT("SteamPath"), folder))
			{
				FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\WOW6432Node\\Valve\\Steam"), TEXT("InstallPath"), folder);
			}
			return folder.Replace(TEXT("\\"), TEXT("/"));
		}

		/** Steam's library folders, from steamapps/libraryfolders.vdf, with Steam's own folder first. */
		TArray<FString> GetSteamLibraryFolders()
		{
			TArray<FString> result;
			const FString steamFolder = GetSteamFolder();
			if (steamFolder.IsEmpty())
			{
				return result;
			}
			result.Add(steamFolder);

			// Each library has a line such as:		"path"		"D:\\SteamLibrary"
			FString libraryFolders;
			if (FFileHelper::LoadFileToString(libraryFolders, *FPaths::Combine(steamFolder, TEXT("steamapps/libraryfolders.vdf"))))
			{
				const FRegexPattern pattern(TEXT("\"path\"\\s+\"([^\"]+)\""));
				FRegexMatcher matcher(pattern, libraryFolders);
				while (matcher.FindNext())
				{
					const FString folder = matcher.GetCaptureGroup(1).Replace(TEXT("\\\\"), TEXT("/")).Replace(TEXT("\\"), TEXT("/"));
					if (!result.ContainsByPredicate([&folder](const FString& existing) { return FPaths::IsSamePath(existing, folder); }))
					{
						result.Add(folder);
					}
				}
			}
			return result;
		}
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

		/** The manifest one locator finds, or an empty string. */
		FString FindWithLocator(const FDrsXrManifestLocator& locator)
		{
#if PLATFORM_WINDOWS
			switch (locator.Locator)
			{
			case EDrsXrManifestLocator::EnvironmentVariable:
			{
				// e.g. OculusBase, set by the Meta Horizon Link installer to "C:\Program Files\Meta Horizon\".
				const FString folder = FPlatformMisc::GetEnvironmentVariable(*locator.Name);
				return folder.IsEmpty() ? FString() : ExistingFile(FPaths::ConvertRelativePathToFull(FPaths::Combine(folder, locator.Path)));
			}

			case EDrsXrManifestLocator::UriHandler:
			{
				// e.g. "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrurlhandler.exe" "%1". The handler
				// can be missing, so a Steam runtime should also have a SteamLibrary locator.
				const FString handler = GetQuotedPath(GetUriProtocolCommand(locator.Name));
				return handler.IsEmpty() ? FString() : ExistingFile(FPaths::Combine(FPaths::GetPath(handler), locator.Path));
			}

			case EDrsXrManifestLocator::SteamLibrary:
				for (const FString& library : GetSteamLibraryFolders())
				{
					const FString manifest = ExistingFile(FPaths::Combine(library, locator.Path));
					if (!manifest.IsEmpty())
					{
						return manifest;
					}
				}
				return FString();

			case EDrsXrManifestLocator::ProgramFiles:
			{
				const FString programFiles = FPlatformMisc::GetEnvironmentVariable(TEXT("ProgramFiles"));
				return programFiles.IsEmpty() ? FString() : ExistingFile(FPaths::Combine(programFiles.Replace(TEXT("\\"), TEXT("/")), locator.Path));
			}

			case EDrsXrManifestLocator::RegisteredRuntime:
				for (const FString& registered : GetRegisteredRuntimeJsons())
				{
					if (FPaths::GetCleanFilename(registered).Equals(locator.Path, ESearchCase::IgnoreCase) && FPaths::FileExists(registered))
					{
						return registered;
					}
				}
				return FString();

			default:
				return FString();
			}
#else
			return FString();
#endif
		}
	}

	FString FindRuntimeJson(const FDrsXrRuntime& runtime)
	{
		for (const FDrsXrManifestLocator& locator : runtime.Manifest)
		{
			const FString manifest = FindWithLocator(locator);
			if (!manifest.IsEmpty())
			{
				return manifest;
			}
		}
		return FString();
	}

	bool IsRuntimeRunning(const FDrsXrRuntime& runtime)
	{
#if PLATFORM_WINDOWS
		const TSet<FString> running = GetRunningProcessNames();
		return runtime.RunningProcesses.ContainsByPredicate([&running](const FString& processName) { return running.Contains(processName.ToLower()); });
#else
		return false;
#endif
	}

	void SelectRuntime()
	{
#if PLATFORM_WINDOWS && !UE_SERVER
		// The rule the engine applies to ClientOnly modules: no runtime on a dedicated server.
		if (IsRunningDedicatedServer())
		{
			return;
		}

		// Without an HMD there is no runtime to choose. The vendor plugin override is not consulted: runtime and plugin are
		// chosen independently, and FXrSelection::IsConsistent reports a mismatch.
		if (FParse::Param(FCommandLine::Get(), TEXT("noHMD")))
		{
			DRS_LOG(LogDrsXrRuntime, Log, "-noHMD given; selecting no OpenXR runtime.");
			return;
		}

		const TArray<FDrsXrRuntime> runtimes = Drs::XrArbiter::GetPrioritisedRuntimes();
		FString selectedJson;
		const FDrsXrRuntime* selected = Drs::XrArbiter::ChooseRuntime(runtimes,
			[](const FDrsXrRuntime& runtime)
			{
				const FString runtimeJson = FindRuntimeJson(runtime);
				DRS_CLOG(!runtimeJson.IsEmpty(), LogDrsXrRuntime, Log, "Runtime '%s' installed at '%s'.", *runtime.Name.ToString(), *runtimeJson);
				return runtimeJson;
			},
			[](const FDrsXrRuntime& runtime)
			{
				const bool isRunning = IsRuntimeRunning(runtime);
				DRS_CLOG(isRunning, LogDrsXrRuntime, Log, "Runtime '%s' is already running.", *runtime.Name.ToString());
				return isRunning;
			},
			selectedJson);

		if (selected == nullptr)
		{
			DRS_LOG(LogDrsXrRuntime, Log, "None of the %d known OpenXR runtimes is installed; using the system's active OpenXR runtime.", runtimes.Num());
			return;
		}

		const FString previousRuntimeJson = FPlatformMisc::GetEnvironmentVariable(RuntimeJsonVariable);
		if (!previousRuntimeJson.IsEmpty() && previousRuntimeJson != selectedJson)
		{
			DRS_LOG(LogDrsXrRuntime, Log, "Overriding %s, which was '%s'.", RuntimeJsonVariable, *previousRuntimeJson);
		}

		FPlatformMisc::SetEnvironmentVar(RuntimeJsonVariable, *selectedJson);
		Drs::XrArbiter::SetXrRuntime(selected->Name, selected->Vendor);
		DRS_LOG(LogDrsXrRuntime, Log, "Selected the %s OpenXR runtime: %s", *selected->Name.ToString(), *selectedJson);
#endif
	}
}
