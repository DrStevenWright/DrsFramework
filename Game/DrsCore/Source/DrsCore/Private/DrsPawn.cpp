// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsPawn.h"
#include "Camera/CameraComponent.h"
#include "DrsInteractorComponent.h"
#include "DrsLocomotionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsPawn)

ADrsPawn::ADrsPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	TrackingOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("TrackingOrigin"));
	TrackingOrigin->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(TrackingOrigin);
	Camera->bLockToHmd = true;

	Interactor = CreateDefaultSubobject<UDrsInteractorComponent>(TEXT("Interactor"));
	Locomotion = CreateDefaultSubobject<UDrsLocomotionComponent>(TEXT("Locomotion"));
}
