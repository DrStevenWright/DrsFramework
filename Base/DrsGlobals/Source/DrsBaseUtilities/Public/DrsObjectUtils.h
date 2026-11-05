// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs
{
	/** Returns "ClassName:ObjectName" for log output, or "None" for a null object. */
	DRSBASEUTILITIES_API FString GetDebugName(const UObject* Object);
}
