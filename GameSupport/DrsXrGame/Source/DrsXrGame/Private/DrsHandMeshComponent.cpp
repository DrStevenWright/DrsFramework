// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsHandMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MotionControllerComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsHandMeshComponent)

namespace
{
	// /Engine/EngineMeshes/SkeletalCube is a 25.2 cm cube (bounds extent 12.6 cm), centred in X and Y, with its base at
	// the origin. Grip-pose axes: X forward out of the fist, Y right, Z up.
	// The default box is roughly a flat hand held vertically: 16 cm long, 3 cm thick, 9 cm tall, centred 4 cm forward.
	constexpr double SkeletalCubeSize = 25.1968;
	const FVector DefaultHandSize(16.0, 3.0, 9.0);
	const FVector DefaultHandScale = DefaultHandSize / SkeletalCubeSize;
	const FVector DefaultHandOffset(4.0, 0.0, -0.5 * DefaultHandSize.Z);
	const FName ColorParameterName(TEXT("Color"));
}

UDrsHandMeshComponent::UDrsHandMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> cubeMesh(TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube"));
	// Engine materials such as BasicShapeMaterial aren't flagged for skeletal meshes, so the plugin has its own.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> placeholderMaterial(TEXT("/DrsXrGame/M_DrsHandPlaceholder.M_DrsHandPlaceholder"));
	if (cubeMesh.Succeeded())
	{
		SetSkeletalMeshAsset(cubeMesh.Object);
	}
	if (placeholderMaterial.Succeeded())
	{
		SetMaterial(0, placeholderMaterial.Object);
	}

	SetRelativeScale3D(DefaultHandScale);
	SetRelativeLocation(DefaultHandOffset);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	CanCharacterStepUpOn = ECB_No;
	CastShadow = true;
}

void UDrsHandMeshComponent::SetHand(EDrsHand hand)
{
	Hand = hand;
	if (HasBegunPlay())
	{
		ApplyColor();
	}
}

FLinearColor UDrsHandMeshComponent::GetDefaultHandColor(EDrsHand hand)
{
	return hand == EDrsHand::Left ? FLinearColor(0.1f, 0.35f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.1f);
}

void UDrsHandMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyColor();
}

void UDrsHandMeshComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (bHideWhenUntracked)
	{
		const UMotionControllerComponent* controller = Cast<UMotionControllerComponent>(GetAttachParent());
		const bool isTracked = controller && controller->IsTracked();
		if (IsVisible() != isTracked)
		{
			SetVisibility(isTracked);
		}
	}
}

void UDrsHandMeshComponent::ApplyColor()
{
	if (UMaterialInstanceDynamic* material = CreateDynamicMaterialInstance(0))
	{
		material->SetVectorParameterValue(ColorParameterName, GetDefaultHandColor(Hand));
	}
}
