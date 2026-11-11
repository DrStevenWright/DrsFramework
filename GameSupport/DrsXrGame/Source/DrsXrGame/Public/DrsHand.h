// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsHand.generated.h"

/** Which of the local user's hands something represents. */
UENUM(BlueprintType)
enum class EDrsHand : uint8
{
	Left,
	Right
};
