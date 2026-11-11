// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs::Graphics
{
	/** Returns a distinct, saturated colour for an index, for debug drawing of many items. */
	DRSGRAPHICSUTILITIES_API FLinearColor GetDebugColor(int32 index);
}
