// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrGameSupportSubsystem.h"
#include "DrsXrPlatformSubsystem.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrGameSupportSubsystem)

bool UDrsXrGameSupportSubsystem::IsXrActive() const
{
	const UDrsXrPlatformSubsystem* platform = GEngine ? GEngine->GetEngineSubsystem<UDrsXrPlatformSubsystem>() : nullptr;
	return platform && platform->IsXrSystemActive();
}

bool UDrsXrGameSupportSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
	return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}
