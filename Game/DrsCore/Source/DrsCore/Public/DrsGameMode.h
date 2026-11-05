// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DrsGameMode.generated.h"

/** Framework game mode (server only). Sets the framework's default game classes. */
UCLASS()
class DRSCORE_API ADrsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADrsGameMode();
};
