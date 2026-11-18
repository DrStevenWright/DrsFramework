// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsCore/DrsGameMode.h"

#include "DrsCore/DrsGameState.h"
#include "DrsCore/DrsPawn.h"
#include "DrsCore/DrsPlayerController.h"
#include "DrsCore/DrsPlayerState.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsGameMode)

ADrsGameMode::ADrsGameMode()
{
	GameStateClass = ADrsGameState::StaticClass();
	PlayerStateClass = ADrsPlayerState::StaticClass();
	PlayerControllerClass = ADrsPlayerController::StaticClass();
	DefaultPawnClass = ADrsPawn::StaticClass();
}
