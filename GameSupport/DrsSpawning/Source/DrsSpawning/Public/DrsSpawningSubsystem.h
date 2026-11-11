// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DrsSpawningSubsystem.generated.h"

/** Per-world spawning service. Placeholder. */
UCLASS()
class DRSSPAWNING_API UDrsSpawningSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;
};
