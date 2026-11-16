// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrControllerType.h"
#include "DrsLog.h"
#include "DrsXrPlatform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrControllerType)

namespace
{
	/** Known hand-tracking interaction profiles. */
	const TSet<FString> HandInteractionProfiles =
	{
		TEXT("/interaction_profiles/ext/hand_interaction_ext"),
		TEXT("/interaction_profiles/microsoft/hand_interaction"),
	};
}

EDrsXrControllerMode Drs::XrInteractionProfiles::DetermineControllerMode(const FString& profile)
{
	if (profile.IsEmpty())
	{
		return EDrsXrControllerMode::Unknown;
	}

	if (HandInteractionProfiles.Contains(profile))
	{
		return EDrsXrControllerMode::TrackedHand;
	}

	// Treat unknown hand profiles (e.g. from a future vendor extension) as hand profiles too, but log them so that they
	// get added to the list above.
	FString profileName;
	profile.Split(TEXT("/"), nullptr, &profileName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (profileName.Contains(TEXT("hand")))
	{
		DRS_LOG(LogDrsXrPlatform, Warning, "Treating the unknown interaction profile '%s' as a tracked-hand profile.", *profile);
		return EDrsXrControllerMode::TrackedHand;
	}

	return EDrsXrControllerMode::PhysicalController;
}

FString FDrsXrControllerType::ToString() const
{
	const UEnum* modeEnum = StaticEnum<EDrsXrControllerMode>();
	return FString::Printf(TEXT("Device name: '%s', Left: '%s' (%s), Right: '%s' (%s)"),
		*DeviceName.ToString(),
		*Left.PhysicalControllerProfilePath, *modeEnum->GetNameStringByValue(static_cast<int64>(Left.Mode)),
		*Right.PhysicalControllerProfilePath, *modeEnum->GetNameStringByValue(static_cast<int64>(Right.Mode)));
}
