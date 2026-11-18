// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrArbiterTypes.generated.h"

/** Whether the vendor plugin types may be selected in this build. */
#define DRS_ENABLE_META_XR ((PLATFORM_WINDOWS || PLATFORM_ANDROID) && !UE_SERVER)
#define DRS_ENABLE_PICO_XR ((PLATFORM_WINDOWS || PLATFORM_ANDROID) && !UE_SERVER)

/**
 * A vendor XR plugin type. The arbiter loads the modules tagged with the selected type (see DrsXrPlatform/CLAUDE.md).
 * When adding a type, also name it in DrsXrArbiterTypes.cpp.
 */
UENUM(BlueprintType)
enum class EDrsXrPluginType : uint8
{
	None = 0,
	Meta,
	Pico,
};

/** How the plugin type was forced, if it was. */
UENUM(BlueprintType)
enum class EDrsXrPluginOverrideType : uint8
{
	None = 0,
	EditorSettings,
	ProjectSettings,
	CommandLine,
};

/** The OpenXR runtime in use. On desktop DrsXrRuntime selects it; on Android the device provides it. */
UENUM(BlueprintType)
enum class EDrsXrRuntimeType : uint8
{
	None = 0,
	Android,
	Oculus,
	Pico,
	SteamVR,
};

namespace Drs::XrArbiter
{
	/** Behaviours or limitations of the selected runtime or vendor plugin. */
	enum class EXrPlatformProperty : uint32
	{
		/** Causes stability issues if selected while no HMD is connected. */
		NonVrNotSupported = 0,
		/** The motion controller transform becomes unstable while the app is out of XR focus (e.g. the system menu is open). */
		ControllerLosesFocus,
	};

	/** The runtime flag bit for a platform property. */
	constexpr uint32 ToRuntimeFlag(EXrPlatformProperty property) { return 1u << static_cast<uint32>(property); }
}

/** How an arbitration condition is evaluated. */
UENUM()
enum class EDrsArbitrationConditionType : uint8
{
	/** All RequiredTrue conditions must be satisfied. */
	RequiredTrue,
	/** At least one optional condition must hold: an OptionalTrue that is satisfied, or an OptionalFalse that is not. */
	OptionalTrue,
	/** All RequiredFalse conditions must not be satisfied. */
	RequiredFalse,
	/** See OptionalTrue. */
	OptionalFalse,
	Invalid UMETA(Hidden, DisplayName = "Not set")
};

/** The property an arbitration condition tests. */
UENUM()
enum class EDrsArbitrationCriteria : uint8
{
	/** (Tethered only) HMD system name, or part of it. */
	DeviceName,
	/** (Standalone only) Device make, or part of it. */
	DeviceMake,
	/** (Standalone only) Device model, or part of it. */
	DeviceModel,
	/** (Standalone only) GPU family, or part of it (e.g. "Adreno (TM) 650"). */
	GpuFamily,
	/** (Tethered only) Active OpenXR runtime. */
	XrRuntime,
	Invalid UMETA(Hidden, DisplayName = "Not set")
};

namespace Drs::XrArbiter
{
	/** The device properties an arbitration rule is evaluated against. Null means unknown. */
	struct FArbitrationInputs
	{
		const FString* DeviceName = nullptr;
		const FString* XrRuntime = nullptr;
		const FString* DeviceMake = nullptr;
		const FString* DeviceModel = nullptr;
		const FString* GpuFamily = nullptr;
	};
}

/** One condition of an arbitration rule. */
USTRUCT(BlueprintType)
struct DRSXRARBITER_API FDrsArbitrationCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arbitration")
	EDrsArbitrationConditionType ConditionType = EDrsArbitrationConditionType::Invalid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arbitration")
	EDrsArbitrationCriteria Criteria = EDrsArbitrationCriteria::Invalid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arbitration")
	FString Value;

	bool IsValid() const { return ConditionType != EDrsArbitrationConditionType::Invalid && Criteria != EDrsArbitrationCriteria::Invalid && !Value.IsEmpty(); }

	/** Whether the tested property contains Value. Unset when that property is unknown. */
	TOptional<bool> Evaluate(const Drs::XrArbiter::FArbitrationInputs& inputs) const;

	/** Parses "ConditionType=...,Criteria=...,Value=..." (the inner text of one condition in config). */
	static FDrsArbitrationCondition ParseFromConfig(const FString& conditionString);
};

/** The conditions under which the arbiter selects a plugin type. */
USTRUCT(BlueprintType)
struct DRSXRARBITER_API FDrsArbitrationRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arbitration")
	EDrsXrPluginType Configuration = EDrsXrPluginType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arbitration")
	TArray<FDrsArbitrationCondition> Conditions;

	bool IsValid() const { return Configuration != EDrsXrPluginType::None && !Conditions.IsEmpty(); }

	bool Evaluate(const Drs::XrArbiter::FArbitrationInputs& inputs) const;

	/**
	 * Parses one rule from its config text, "(Configuration=Meta,Conditions=((...),(...)))". The arbiter reads config
	 * directly because it decides before the settings object is loaded.
	 */
	static FDrsArbitrationRule ParseFromConfig(const FString& ruleString);

	/** Reads and parses every valid rule in an array property of a config section. */
	static TArray<FDrsArbitrationRule> ParseFromConfig(const TCHAR* section, const TCHAR* property, const FString& filename);
};
