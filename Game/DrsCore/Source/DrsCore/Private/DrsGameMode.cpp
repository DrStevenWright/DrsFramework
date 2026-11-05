// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsGameMode.h"
#include "DrsGameState.h"
#include "DrsPawn.h"
#include "DrsPlayerController.h"
#include "DrsPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsGameMode)

ADrsGameMode::ADrsGameMode()
{
	GameStateClass = ADrsGameState::StaticClass();
	PlayerStateClass = ADrsPlayerState::StaticClass();
	PlayerControllerClass = ADrsPlayerController::StaticClass();
	DefaultPawnClass = ADrsPawn::StaticClass();
}
