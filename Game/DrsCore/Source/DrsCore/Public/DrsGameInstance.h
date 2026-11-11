// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DrsGameInstance.generated.h"

/** Framework game instance, one per running game (client or server). Lives across map changes; not replicated. */
UCLASS()
class DRSCORE_API UDrsGameInstance : public UGameInstance
{
	GENERATED_BODY()
};
