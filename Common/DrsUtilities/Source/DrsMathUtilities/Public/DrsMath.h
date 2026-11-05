// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs::Math
{
	/** Keeps only the yaw of a rotation, e.g. to get a level heading from an HMD orientation. */
	DRSMATHUTILITIES_API FQuat ToYawOnly(const FQuat& Rotation);
}
