// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsPawn.h"
#include "Camera/CameraComponent.h"
#include "DrsCore.h"
#include "DrsHandMeshComponent.h"
#include "DrsLog.h"
#include "DrsXrPlatformSubsystem.h"
#include "Engine/Engine.h"
#include "MotionControllerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsPawn)

namespace
{
	const FName LeftGripSource(TEXT("LeftGrip"));
	const FName RightGripSource(TEXT("RightGrip"));
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
}

void ADrsPawn::BeginPlay()
{
	Super::BeginPlay();

	// The tracking origin is engine-global, so only the locally viewed pawn should set it.
	// The pawn is not possessed yet at BeginPlay, so check the net mode rather than the controller.
	if (GetNetMode() != NM_DedicatedServer && GEngine)
	{
		if (UDrsXrPlatformSubsystem* xrPlatform = GEngine->GetEngineSubsystem<UDrsXrPlatformSubsystem>())
		{
			if (!xrPlatform->SetFloorTrackingOrigin())
			{
				DRS_LOG(LogDrsCore, Log, "No XR system active; running without HMD tracking.");
			}
		}
	}
}
