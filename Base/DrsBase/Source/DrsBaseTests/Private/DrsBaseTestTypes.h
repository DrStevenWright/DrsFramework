// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsBaseTestTypes.generated.h"

/** A reflected enum class for the votable state tests. */
UENUM()
enum class EDrsBaseTestVoter : uint8
{
	First,
	Second
};

/** An enum without reflection, which the votable state shows as a number. */
enum class EDrsBaseTestPlainVoter : uint8
{
	First,
	Second
};
