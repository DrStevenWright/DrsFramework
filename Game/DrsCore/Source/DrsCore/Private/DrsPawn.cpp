// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsPawn.h"
#include "Camera/CameraComponent.h"
#include "DrsCore.h"
#include "DrsEnhancedInputComponent.h"
#include "DrsHandMeshComponent.h"
#include "DrsInputMappingContext.h"
#include "DrsInputUtils.h"
#include "DrsLog.h"
#include "DrsMotionControllerInputComponent.h"
#include "DrsXrPlatformLibrary.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "MotionControllerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsPawn)

namespace
{
	const FName LeftGripSource(TEXT("LeftGrip"));
	const FName RightGripSource(TEXT("RightGrip"));

	// Reason for adding the motion controller input context, and the priority to add it at. The context is fundamental,
	// so nothing may override it.
	const FName MotionControllerContextReason(TEXT("DrsPawnMotionController"));
	constexpr int32 MotionControllerContextPriority = TNumericLimits<int32>::Max();

	// Gets the Enhanced Input subsystem of the local player that controls the pawn, if there is one.
	UEnhancedInputLocalPlayerSubsystem* GetControllingInputSubsystem(const APawn* pawn)
	{
		const APlayerController* controller = Cast<APlayerController>(pawn->GetController());
		return controller ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(controller->GetLocalPlayer()) : nullptr;
	}
}

ADrsPawn::ADrsPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	TrackingOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("TrackingOrigin"));
	TrackingOrigin->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(TrackingOrigin);
	Camera->bLockToHmd = true;

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(TrackingOrigin);
	LeftController->SetTrackingMotionSource(LeftGripSource);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(TrackingOrigin);
	RightController->SetTrackingMotionSource(RightGripSource);

	LeftHand = CreateDefaultSubobject<UDrsHandMeshComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(LeftController);
	LeftHand->SetHand(EDrsHand::Left);

	RightHand = CreateDefaultSubobject<UDrsHandMeshComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(RightController);
	RightHand->SetHand(EDrsHand::Right);

	LeftMotionInput = CreateDefaultSubobject<UDrsMotionControllerInputComponent>(TEXT("LeftMotionInput"));
	LeftMotionInput->SetHand(EDrsHand::Left);

	RightMotionInput = CreateDefaultSubobject<UDrsMotionControllerInputComponent>(TEXT("RightMotionInput"));
	RightMotionInput->SetHand(EDrsHand::Right);
}

void ADrsPawn::BeginPlay()
{
	Super::BeginPlay();

	// The tracking origin is engine-global, so only the locally viewed pawn should set it.
	// The pawn is not possessed yet at BeginPlay, so check the net mode rather than the controller.
	if (GetNetMode() != NM_DedicatedServer && !UDrsXrPlatformLibrary::SetFloorTrackingOrigin())
	{
		DRS_LOG(LogDrsCore, Log, "No XR system active; running without HMD tracking.");
	}
}

void ADrsPawn::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	RemoveMotionControllerContext();

	Super::EndPlay(endPlayReason);
}

void ADrsPawn::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);

	// The engine only calls this for a pawn that is controlled by a local player, so the controller is known here and
	// the context goes to that player's subsystem.
	AddMotionControllerContext();

	UDrsEnhancedInputComponent* drsInput = Cast<UDrsEnhancedInputComponent>(playerInputComponent);
	if (!drsInput)
	{
		DRS_LOG(LogDrsCore, Error, "The input component is not a UDrsEnhancedInputComponent, so motion controller input will not be forwarded. Set DefaultInputComponentClass in the project's input settings.");
		return;
	}

	LeftMotionInput->AddInputBindings(drsInput, MotionControllerContext);
	RightMotionInput->AddInputBindings(drsInput, MotionControllerContext);
}

void ADrsPawn::UnPossessed()
{
	// The input component is destroyed along with the possession, so remove the bindings (which also releases any
	// forwarded keys) and the context while the controller is still known.
	UDrsEnhancedInputComponent* drsInput = Cast<UDrsEnhancedInputComponent>(InputComponent);
	LeftMotionInput->RemoveInputBindings(drsInput);
	RightMotionInput->RemoveInputBindings(drsInput);
	RemoveMotionControllerContext();

	Super::UnPossessed();
}

void ADrsPawn::AddMotionControllerContext()
{
	if (!MotionControllerContext)
	{
		DRS_LOG(LogDrsCore, Warning, "No motion controller input context is set, so motion controller input will not be forwarded.");
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* subsystem = GetControllingInputSubsystem(this))
	{
		Drs::Input::AddInputContextToSubsystem(subsystem, MotionControllerContext, MotionControllerContextPriority, MotionControllerContextReason);
		bMotionControllerContextAdded = true;
	}
}

void ADrsPawn::RemoveMotionControllerContext()
{
	if (bMotionControllerContextAdded)
	{
		Drs::Input::RemoveInputContextFromSubsystem(GetControllingInputSubsystem(this), MotionControllerContext, MotionControllerContextReason);
		bMotionControllerContextAdded = false;
	}
}
