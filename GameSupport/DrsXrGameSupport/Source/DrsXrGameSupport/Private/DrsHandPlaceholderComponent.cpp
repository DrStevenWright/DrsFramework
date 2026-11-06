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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CubeMesh.Succeeded())
	{
		SetStaticMesh(CubeMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		SetMaterial(0, ShapeMaterial.Object);
	}

	SetRelativeScale3D(PlaceholderScale);
	SetRelativeLocation(PlaceholderOffset);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	CanCharacterStepUpOn = ECB_No;
	CastShadow = true;
}

void UDrsHandPlaceholderComponent::SetHand(EDrsHand InHand)
{
	Hand = InHand;
	if (HasBegunPlay())
	{
		ApplyColor();
	}
}

FLinearColor UDrsHandPlaceholderComponent::GetDefaultHandColor(EDrsHand InHand)
{
	return InHand == EDrsHand::Left ? FLinearColor(0.1f, 0.35f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.1f);
}

void UDrsHandPlaceholderComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyColor();
}

void UDrsHandPlaceholderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHideWhenUntracked)
	{
		const UMotionControllerComponent* Controller = Cast<UMotionControllerComponent>(GetAttachParent());
		const bool bTracked = Controller && Controller->IsTracked();
		if (IsVisible() != bTracked)
		{
			SetVisibility(bTracked);
		}
	}
}

void UDrsHandPlaceholderComponent::ApplyColor()
{
	if (UMaterialInstanceDynamic* Material = CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(ColorParameterName, GetDefaultHandColor(Hand));
	}
}
