// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsBase/DrsHand.h"

#include "DrsXrControllerType.generated.h"

/** How a hand is controlled: by a physical motion controller, or as a tracked hand. */
UENUM(BlueprintType)
enum class EDrsXrControllerMode : uint8
{
	/** Not known yet (e.g. before the runtime reports an interaction profile). */
	Unknown,
	PhysicalController,
	TrackedHand,
};

/** The controller state of one hand. Local-only: holds no replicated state. */
USTRUCT(BlueprintType)
struct DRSXRPLATFORM_API FDrsXrControllerState
{
	GENERATED_BODY()

	/** The hand's current OpenXR interaction profile. Don't use it to identify the headset model. */
	UPROPERTY(BlueprintReadOnly, Category = "XR")
	FString ProfilePath;

	/** The last interaction profile of a physical motion controller, if any. Don't use it to identify the headset model. */
	UPROPERTY(BlueprintReadOnly, Category = "XR")
	FString PhysicalControllerProfilePath;

	/** The decided controller mode. */
	UPROPERTY(BlueprintReadOnly, Category = "XR")
	EDrsXrControllerMode Mode = EDrsXrControllerMode::PhysicalController;

	/** The mode the current interaction profile indicates, which can differ from the decided mode. */
	EDrsXrControllerMode ProfileMode = EDrsXrControllerMode::Unknown;

	bool operator==(const FDrsXrControllerState& other) const { return ProfilePath == other.ProfilePath && PhysicalControllerProfilePath == other.PhysicalControllerProfilePath && Mode == other.Mode; }
	bool operator!=(const FDrsXrControllerState& other) const { return !(*this == other); }
};

/** The controllers of both hands and their modes, on the local client. Local-only: holds no replicated state. */
USTRUCT(BlueprintType)
struct DRSXRPLATFORM_API FDrsXrControllerType
{
	GENERATED_BODY()

	bool IsValid() const { return !DeviceName.IsNone(); }

	const FDrsXrControllerState& GetHand(EDrsHand hand) const { return (hand == EDrsHand::Left) ? Left : Right; }
	bool IsTrackedHand(EDrsHand hand) const { return GetHand(hand).Mode == EDrsXrControllerMode::TrackedHand; }
	bool HasPhysicalControllerProfile(EDrsHand hand) const { return !GetHand(hand).PhysicalControllerProfilePath.IsEmpty(); }

	bool operator==(const FDrsXrControllerType& other) const { return DeviceName == other.DeviceName && Left == other.Left && Right == other.Right; }
	bool operator!=(const FDrsXrControllerType& other) const { return !(*this == other); }

	FString ToString() const;

	/** The HMD device name, which tells apart models that share a controller type (e.g. Quest 2 and Quest 3). */
	UPROPERTY(BlueprintReadOnly, Category = "XR")
	FName DeviceName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "XR")
	FDrsXrControllerState Left;

	UPROPERTY(BlueprintReadOnly, Category = "XR")
	FDrsXrControllerState Right;
};

namespace Drs::XrInteractionProfiles
{
	/** The controller mode an interaction profile path indicates (Unknown for an empty path). */
	DRSXRPLATFORM_API EDrsXrControllerMode DetermineControllerMode(const FString& profile);
}
