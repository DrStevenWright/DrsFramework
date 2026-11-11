// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrGameSubsystem.h"
#include "DrsXrPlatformSubsystem.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrGameSubsystem)

bool UDrsXrGameSubsystem::IsXrActive() const
{
	const UDrsXrPlatformSubsystem* platform = GEngine ? GEngine->GetEngineSubsystem<UDrsXrPlatformSubsystem>() : nullptr;
	return platform && platform->IsXrSystemActive();
}

bool UDrsXrGameSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
	return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}
