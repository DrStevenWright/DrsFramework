// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrGameSupportSubsystem.h"
#include "DrsXrPlatformSubsystem.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrGameSupportSubsystem)

bool UDrsXrGameSupportSubsystem::IsXrActive() const
{
	const UDrsXrPlatformSubsystem* Platform = GEngine ? GEngine->GetEngineSubsystem<UDrsXrPlatformSubsystem>() : nullptr;
	return Platform && Platform->IsXrSystemActive();
}

bool UDrsXrGameSupportSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
