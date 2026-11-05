// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsSpawningSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsSpawningSubsystem)

bool UDrsSpawningSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
