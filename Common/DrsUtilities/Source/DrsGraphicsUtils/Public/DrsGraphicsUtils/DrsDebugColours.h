// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs::Graphics
{
	/** Returns a distinct, saturated colour for an index, for debug drawing of many items. */
	DRSGRAPHICSUTILS_API FLinearColor GetDebugColour(int32 index);
}
