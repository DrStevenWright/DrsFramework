// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiterTypes.generated.h"

/** Whether MetaXR code may be compiled in this build. */
#define DRS_ENABLE_META_XR ((PLATFORM_WINDOWS || PLATFORM_ANDROID) && !UE_SERVER)

/** How the plugin type was forced, if it was. */
UENUM(BlueprintType)
enum class EDrsXrPluginOverrideType : uint8
{
	None = 0,
	EditorSettings,
	ProjectSettings,
	CommandLine,
};

/** A vendor XR plugin, such as Meta or Pico, and the words that identify its hardware. */
USTRUCT(BlueprintType)
struct DRSXRARBITER_API FDrsXrVendor
{
	GENERATED_BODY()

	/** The vendor's name, as used in plugin descriptors (ArbitratedXrPlugin), overrides and on the command line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DRS|XR Platform")
	FName Id;

	/** Words that identify the vendor's hardware in a device name, brand or manufacturer. Not case-sensitive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DRS|XR Platform")
	TArray<FString> Keywords;

	/** A module that is loaded whenever the vendor's plugin is running, used to tell which vendor plugin is in use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DRS|XR Platform")
	FName MarkerModule;

	bool IsValid() const { return !Id.IsNone(); }
};

/** A way to find an OpenXR runtime's manifest file on Windows. */
UENUM()
enum class EDrsXrManifestLocator : uint8
{
	/** Path is relative to the folder in the environment variable Name. */
	EnvironmentVariable,
	/** Path is relative to the folder of the program that handles the URI protocol Name. */
	UriHandler,
	/** Path is relative to each of Steam's library folders. */
	SteamLibrary,
	/** Path is relative to Program Files. */
	ProgramFiles,
	/** Path is a manifest file name, looked for among the runtimes registered with the OpenXR loader. */
	RegisteredRuntime,
	Invalid UMETA(Hidden, DisplayName = "Not set")
};

/** One place to look for an OpenXR runtime's manifest. */
USTRUCT()
struct DRSXRARBITER_API FDrsXrManifestLocator
{
	GENERATED_BODY()

	/** How to find the folder that Path is relative to. */
	UPROPERTY(EditAnywhere, Category = "DRS|XR Platform")
	EDrsXrManifestLocator Locator = EDrsXrManifestLocator::Invalid;

	/** The environment variable or URI protocol, for the locators that need one. */
	UPROPERTY(EditAnywhere, Category = "DRS|XR Platform")
	FString Name;

	/** The manifest's path, or its file name for RegisteredRuntime. */
	UPROPERTY(EditAnywhere, Category = "DRS|XR Platform")
	FString Path;

	bool IsValid() const { return Locator != EDrsXrManifestLocator::Invalid && !Path.IsEmpty(); }
};

/** A desktop OpenXR runtime that can be selected at startup. */
USTRUCT()
struct DRSXRARBITER_API FDrsXrRuntime
{
	GENERATED_BODY()

	/** The runtime's name, as used in the runtime priority and in logs. */
	UPROPERTY(EditAnywhere, Category = "DRS|XR Platform")
	FName Name;

	/** The vendor whose hardware the runtime implies. None for a runtime that serves several vendors, such as SteamVR. */
	UPROPERTY(EditAnywhere, Category = "DRS|XR Platform")
	FName Vendor;

	/** Where to look for the runtime's manifest, tried in order. */
	UPROPERTY(EditAnywhere, Category = "DRS|XR Platform")
	TArray<FDrsXrManifestLocator> Manifest;

	/** Program file names. If any of them is running, the runtime is running and is selected over installed ones. */
	UPROPERTY(EditAnywhere, Category = "DRS|XR Platform")
	TArray<FString> RunningProcesses;

	bool IsValid() const { return !Name.IsNone() && !Manifest.IsEmpty(); }
};
