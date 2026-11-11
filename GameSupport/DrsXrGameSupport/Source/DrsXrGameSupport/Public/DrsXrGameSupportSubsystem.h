// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DrsXrGameSupportSubsystem.generated.h"

/** Per-world access point for runtime XR features. */
UCLASS()
class DRSXRGAMESUPPORT_API UDrsXrGameSupportSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** True when XR is active for this game. */
	UFUNCTION(BlueprintPure, Category = "Drs|XR Game Support")
	bool IsXrActive() const;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;
};
