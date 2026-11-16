// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsObjectUtils.h"
#include "UObject/Object.h"

FString Drs::GetDebugName(const UObject* object)
{
	if (!object)
	{
		return TEXT("None");
	}
	return FString::Printf(TEXT("%s:%s"), *GetNameSafe(object->GetClass()), *object->GetName());
}
