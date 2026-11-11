// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsHandPlaceholderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MotionControllerComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsHandPlaceholderComponent)

namespace
{
	// The engine cube is 100 cm per side. Grip-pose axes: X forward out of the fist, Y right, Z up.
	// The box is roughly a flat hand held vertically: 16 cm long, 3 cm thick, 9 cm tall.
	const FVector PlaceholderScale(0.16, 0.03, 0.09);
	const FVector PlaceholderOffset(4.0, 0.0, 0.0);
	const FName ColorParameterName(TEXT("Color"));
}

UDrsHandPlaceholderComponent::UDrsHandPlaceholderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> shapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (cubeMesh.Succeeded())
	{
		SetStaticMesh(cubeMesh.Object);
	}
	if (shapeMaterial.Succeeded())
	{
		SetMaterial(0, shapeMaterial.Object);
	}

	SetRelativeScale3D(PlaceholderScale);
	SetRelativeLocation(PlaceholderOffset);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	CanCharacterStepUpOn = ECB_No;
	CastShadow = true;
}

void UDrsHandPlaceholderComponent::SetHand(EDrsHand hand)
{
	Hand = hand;
	if (HasBegunPlay())
	{
		ApplyColor();
	}
}

FLinearColor UDrsHandPlaceholderComponent::GetDefaultHandColor(EDrsHand hand)
{
	return hand == EDrsHand::Left ? FLinearColor(0.1f, 0.35f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.1f);
}

void UDrsHandPlaceholderComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyColor();
}

void UDrsHandPlaceholderComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
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

void UDrsHandPlaceholderComponent::ApplyColor()
{
	if (UMaterialInstanceDynamic* material = CreateDynamicMaterialInstance(0))
	{
		material->SetVectorParameterValue(ColorParameterName, GetDefaultHandColor(Hand));
	}
}
