// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsObjectUtils.h"
#include "UObject/Object.h"

FString Drs::GetDebugName(const UObject* Object)
{
	if (!Object)
	{
		return TEXT("None");
	}
	return FString::Printf(TEXT("%s:%s"), *GetNameSafe(Object->GetClass()), *Object->GetName());
}
