// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Components/TouchWidgetInteractionComponent.h"

#include "Components/CoreWidgetComponent.h"
#include "Components/WidgetInteractionDebug.h"
#include "Components/WidgetComponent.h"
#include "CoreEnhancedInputComponent.h"
#include "CoreInputMappingContext.h"
#include "CoreInputSettings.h"
#include "Engine/OverlapResult.h"
#include "Interfaces/SurfaceConstraintActuatorInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Types/SurfaceConstraint.h"
#include "Utils/EnumToString.h"
#include "Utils/WidgetUtils.h"


UTouchWidgetInteractionComponent::UTouchWidgetInteractionComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	// UWidgetInteractionComponent
	InteractionSource = EWidgetInteractionSource::Custom;
	InteractionDistance = 10.f; // Not used for custom interaction sources, just setting it to a reasonable value here so that the engine's debug draw is not wildly off.

	// UWidgetInteractionComponent
	bMoreGenerousDragArbitration = true;
}


void UTouchWidgetInteractionComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	// This must run after the base implementation, which performs the trace that identifies the touched widget surface.
	UpdateTouchResistance();
}


#if WITH_EDITOR
bool UTouchWidgetInteractionComponent::CanEditChange(const FProperty* property) const
{
	if (property)
	{
		// Touch interactors build custom hit results against the surfaces identified by GetCustomHoveredWidgetComponent().
		// The interaction source must therefore be fixed at Custom. The interaction distance is only used by the engine's
		// default interaction source and so becomes meaningless here.
		if (property->GetFName() == GET_MEMBER_NAME_CHECKED(UTouchWidgetInteractionComponent, InteractionSource)
			|| property->GetFName() == GET_MEMBER_NAME_CHECKED(UTouchWidgetInteractionComponent, InteractionDistance))
		{
			return false;
		}
	}

	return Super::CanEditChange(property);
}
#endif


void UTouchWidgetInteractionComponent::OnBindingChanged()
{
	Super::OnBindingChanged();

	// Surface constraints are actuated by the interactor rather than by this component, but not every interactor can
	// actuate them.
	SurfaceConstraintActuator = InteractorComponent;
}


void UTouchWidgetInteractionComponent::OnComponentValidate()
{
	if (InteractionSource != EWidgetInteractionSource::Custom)
	{
		ShowValidationWarning(TEXT("InteractionSource should be set to Custom for touch-based widget interaction."));
		InteractionSource = EWidgetInteractionSource::Custom;
	}
	
	if (!(MaxHoverDistance > MinHoverDistance))
	{
		ShowValidationWarning(TEXT("MaxHoverDistance (%.2f) should be larger than MinHoverDistance (%.2f)"), MaxHoverDistance, MinHoverDistance);
	}

	if (!FMath::IsWithinInclusive(MaxSelectDistance, MinHoverDistance, MaxHoverDistance))
	{
		ShowValidationWarning(TEXT("MaxSelectDistance (%.2f) should be somewhere within the hover range [%.2f, %.2f]."), MaxSelectDistance, MinHoverDistance, MaxHoverDistance);
	}

	if (!(MinClearDistance >= MaxSelectDistance))
	{
		ShowValidationWarning(TEXT("MinClearDistance (%.2f) should be larger than MaxSelectDistance (%.2f) to help prevent unintentional selection."), MinClearDistance, MaxSelectDistance);
	}
}


void UTouchWidgetInteractionComponent::AddInputActions(UDrsEnhancedInputComponent* inputComponent)
{
	if (ensure(IsValid(inputComponent)) && InteractorHand.IsSet())
	{
		if (const UDrsInputMappingContext* context = UDrsInputSettings::Get().UIScrollControls.GetContextForHand(InteractorHand.GetValue()))
		{
			inputComponent->BindActionID(context, InputActionID_Scroll_Vertical, ETriggerEvent::Triggered, this, &UTouchWidgetInteractionComponent::OnScrollVertical);
			inputComponent->BindActionID(context, InputActionID_Scroll_Horizontal, ETriggerEvent::Triggered, this, &UTouchWidgetInteractionComponent::OnScrollHorizontal);
		}
	}
}


void UTouchWidgetInteractionComponent::UpdateSelectedState()
{
	// Don't allow selection if the finger is close to the widget when hovering starts or when a previous selection is
	// released. This discourages unintentional selection.
	if (bIsSelecting || !bIsHovering)
	{
		bCanSelect = false;
	}
	else if (HoverDistance > MinClearDistance)
	{
		bCanSelect = true;
	}

	if (!bIsSelecting)
	{
		// Selection is triggered when the fingertip is pressed within some distance of the surface.
		if (bIsHovering && bCanSelect && HoverDistance <= MaxSelectDistance)
		{
			OnSelect();
		}
	}
	else
	{
		// Selection is released when the fingertip clears some distance from the surface.
		if (!bIsHovering || HoverDistance > MinClearDistance)
		{
			OnRelease();
		}
	}
}


void UTouchWidgetInteractionComponent::OnResetState()
{
	Super::OnResetState();
	
	ClearTouchResistance();

	bCanSelect = false;
	bCanDrag = false;
}


bool UTouchWidgetInteractionComponent::CanDrag() const
{
	// Don't allow dragging if the finger is close to the widget when hovering starts. This is to discourage unintentional
	// dragging. Instead, wait for the finger to exceed some clearance distance before allowing dragging to be started.
	if (!Super::CanDrag())
	{
		bCanDrag = false;
	}
	else if (HoverDistance > MinClearDistance)
	{
		bCanDrag = true;
	}

	// Dragging requires the fingertip to be held within some distance of the surface.
	return bCanDrag && HoverDistance <= MaxSelectDistance;
}


bool UTouchWidgetInteractionComponent::IsWithinHoverBand(float hoverDistance) const
{
	// MaxHoverDistance should be >= MaxSelectDistance, but we take measures to handle it in case the error is pending fix.
	return FMath::IsWithinInclusive(hoverDistance, MinHoverDistance, FMath::Max(MaxHoverDistance, MaxSelectDistance));
}


bool UTouchWidgetInteractionComponent::CanHover_Implementation(const UWidgetComponent* widgetComponent) const
{
	const UCoreWidgetComponent* coreWidgetComponent = Cast<UCoreWidgetComponent>(widgetComponent);
	if (coreWidgetComponent && !coreWidgetComponent->CanTouchInteract())
	{
		return false;
	}
	
	if (!IsWithinHoverBand(HoverDistance))
	{
		return false;
	}
	
	// Check whether there is some obstruction between the component and the collision surface. This is applied only to
	// the hovered widget, rather than every candidate in the default implementation of GetCustomHoveredWidgetComponent().
	// It means that it is theoretically possible to have cases where hovering is rejected entirely, even though there is
	// a more distant unoccluded widget elsewhere. Handling this narrow edge case is deemed not worth the performnce cost
	// it would incur.
	FHitResult hit;
	if (UKismetSystemLibrary::LineTraceSingle(this, GetComponentLocation(), GetMousePosition(), UEngineTypes::ConvertToTraceType(ECC_Visibility), true, {widgetComponent->GetOwner()}, EDrawDebugTrace::None, hit, true))
	{
		return false;
	}
	
	return Super::CanHover_Implementation(widgetComponent);
}


FHitResult UTouchWidgetInteractionComponent::CreateCustomHitResult() const
{
	UWidgetComponent* widgetComponent = GetCustomHoveredWidgetComponent();
	if (!IsValid(widgetComponent))
	{
		return Super::CreateCustomHitResult();
	}

	// Find the closest point on the widget's collision surface, reusing existing results where available.
	const FVector componentPos   = GetComponentLocation();
	const bool    useCached      = (CachedClosestWidget.Get() == widgetComponent);
	FVector       impactPos      = useCached ? CachedCollisionPoint : componentPos;
	const float   impactDistance = useCached ? FVector::Dist(componentPos, impactPos) : widgetComponent->GetDistanceToCollision(componentPos, impactPos);

	// Discard/report degenerate hit info.
	if (componentPos.ContainsNaN() || impactPos.ContainsNaN())
	{
		UE_LOG(LogWidgetInteraction, Warning, TEXT("Discarding degenerate touch hit result on %s-hand interactor '%s' - location: %s, impact: %s, distance %.3f, widget: '%s'.")
			, *RW1::EnumToString(GetHand())
			, *GetName()
			, *componentPos.ToString()
			, *impactPos.ToString()
			, impactDistance
			, *GetNameSafe(widgetComponent));
		return Super::CreateCustomHitResult();
	}
	UE_CLOG(impactDistance < 0.0f, LogWidgetInteraction, Verbose, TEXT("Surface distance query failed against widget component '%s'."), *GetNameSafe(widgetComponent));

	const bool isBackHit = ((componentPos - impactPos) | widgetComponent->GetForwardVector()) < 0.0f;
	FVector traceStart	 = isBackHit ? impactPos : componentPos;
	FVector traceEnd	 = isBackHit ? componentPos : impactPos;

	// Handle degenerate trace info that can arise from cylindrical widgets.
	if ((traceEnd - traceStart).IsNearlyZero())
	{
		const FVector surfaceNormal = WidgetUtils::GetWidgetNormalAtPosition(widgetComponent, impactPos);
		const FVector traceDirection = surfaceNormal.IsNearlyZero() ? widgetComponent->GetForwardVector() : surfaceNormal;
		traceStart	= impactPos + traceDirection;
		traceEnd	= impactPos - traceDirection;
	}
	
	FHitResult customHit;
	customHit.HitObjectHandle	= widgetComponent->GetOwner(); 
	customHit.Component			= widgetComponent;
	customHit.TraceStart		= traceStart;
	customHit.TraceEnd			= traceEnd;
	customHit.ImpactPoint		= impactPos;
	customHit.Distance			= impactDistance;
	customHit.bBlockingHit		= true;

	return customHit;
}


UWidgetComponent* UTouchWidgetInteractionComponent::GetCustomHoveredWidgetComponent_Implementation() const
{
	if (InteractionSource != EWidgetInteractionSource::Custom)
	{
		return WeakHoveredWidgetComponent.Get();
	}

	const FVector componentLocation = GetComponentLocation();
	const float   overlapRadius     = FMath::Max(MaxHoverDistance, MaxSelectDistance) + TouchRadius; // Same distance band used by CanHover, adjusted to include touch radius.
	
	TArray<UPrimitiveComponent*> ignoreComponents;
	GetRelatedComponentsToIgnoreInAutomaticHitTesting(ignoreComponents);
	
	FCollisionQueryParams params(SCENE_QUERY_STAT(WidgetInteractionComponentOverlap));
	params.AddIgnoredComponents(ignoreComponents);
	
	// Sphere overlap query to find nearest widget surface.
	TArray<FOverlapResult> overlaps;
	if (const UWorld* world = GetWorld())
	{
		world->OverlapMultiByChannel(overlaps, componentLocation, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(overlapRadius), params);
	}
	
	// Of the widgets in range, choose the one whose collision surface lies closest to this component - the same measure
	// that the hover distance and the surface constraint are subsequently derived from.
	UWidgetComponent* closestWidgetComponent = nullptr;
	float closestWidgetComponentDistance = TNumericLimits<float>::Max();
	FVector closestWidgetComponentPoint = componentLocation;
	
	for (const FOverlapResult& overlap : overlaps)
	{
		// Hidden widget components keep their collision and so must be filtered out here to prevent them occluding
		// interaction with other, visible widgets.
		UWidgetComponent* widgetComponent = Cast<UWidgetComponent>(overlap.GetComponent());
		if (!IsValid(widgetComponent) || !widgetComponent->IsVisible())
		{
			continue;
		}
		
		// Track the widget component whose visible surface this interactor is closest to. Penetrating positions are still
		// ranked properly, even when the distance is clamped at 0. Negative distances, on the other hand, means that the
		// query failed outright and the result can't be used.
		FVector collisionPoint = componentLocation;
		float distance = widgetComponent->GetDistanceToCollision(componentLocation, collisionPoint);
		if (distance < 0.0f)
		{
			continue;
		}
		
		// For curved widgets, the collision point needs to be projected from the collision chord plane onto the arc surface,
		// and the distance adjusted.
		FVector surfacePoint = collisionPoint;
		if (widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder)
		{
			surfacePoint = widgetComponent->GetCylinderHitLocation(collisionPoint, -widgetComponent->GetForwardVector()).Get<0>();
			distance     = FVector::Dist(componentLocation, surfacePoint);
		}

		// Discard candidates that cannot be hovered. This rejects candidates that the interactor is behind, and ensures
		// that hoverable candidates will not be rejected in favor of ones that are not hoverable. This check is measured
		// against the visible surface rather than the collision, which are not the same for curved widgets.
		const bool  isBehindSurface = ((surfacePoint - componentLocation) | widgetComponent->GetForwardVector()) > 0.0f;
		const float hoverDistance   = (isBehindSurface ? -distance : distance) - TouchRadius;
		if (!IsWithinHoverBand(hoverDistance))
		{
			continue;
		}

		if (distance < closestWidgetComponentDistance)
		{
			closestWidgetComponent = widgetComponent;
			closestWidgetComponentDistance = distance;
			closestWidgetComponentPoint = collisionPoint;
		}
	}
	
	// Cache the results of this query so that CreateCustomHitResult() can re-use them asnd save an unnecessary surface
	// collision query.
	CachedClosestWidget  = closestWidgetComponent;
	CachedCollisionPoint = closestWidgetComponentPoint;
	
	return closestWidgetComponent;
}


void UTouchWidgetInteractionComponent::UpdateTouchResistance()
{
	FSurfaceConstraint constraint;
	if (bEnableTouchResistance && SurfaceConstraintActuator && BuildTouchResistanceConstraint(constraint))
	{
		SetIsTouchResisting(SurfaceConstraintActuator->ApplySurfaceConstraint(constraint));
	}
	else
	{
		ClearTouchResistance();
	}
}


void UTouchWidgetInteractionComponent::ClearTouchResistance()
{
	if (SurfaceConstraintActuator)
	{
		SurfaceConstraintActuator->ClearSurfaceConstraint(this);
	}

	// Called here rather than from UpdateTouchResistance, so that OnResetState() reports it too.
	SetIsTouchResisting(false);
}


void UTouchWidgetInteractionComponent::SetIsTouchResisting(bool isResisting)
{
	if (bIsTouchResisting != isResisting)
	{
		bIsTouchResisting = isResisting;
		OnTouchResistanceChanged(isResisting);
	}
}


bool UTouchWidgetInteractionComponent::BuildTouchResistanceConstraint(FSurfaceConstraint& outConstraint)
{
	if (!bIsHovering || !WeakHoveredWidgetComponent.IsValid())
	{
		return false;
	}

	// Disable touch resistance for specific widget components.
	UWidgetComponent* hoveredWidgetComponent = WeakHoveredWidgetComponent.Get();
	const UCoreWidgetComponent* coreWidgetComponent = Cast<UCoreWidgetComponent>(hoveredWidgetComponent);
	if (coreWidgetComponent && !coreWidgetComponent->IsTouchResistanceEnabled())
	{
		return false;
	}
	
	// Override error tolerance for specific widget components.
	const float errorTolerance = (coreWidgetComponent && coreWidgetComponent->GetTouchResistanceErrorTolerance() >= 0.f) ? coreWidgetComponent->GetTouchResistanceErrorTolerance() : TouchResistanceErrorTolerance;
	
	// Configure the surface constraint.
	outConstraint.ConstrainedComponent = this;
	outConstraint.SurfaceComponent     = hoveredWidgetComponent;
	outConstraint.ErrorTolerance       = errorTolerance;

	switch (WeakHoveredWidgetComponent->GetGeometryMode())
	{
	case EWidgetGeometryMode::Plane:
		{
			// The clamp plane is derived from the widget component itself rather than from the hit result, because the
			// hit result stops describing the surface at precisely the moment touch resistance needs it - once the query
			// point penetrates the collision, GetDistanceToCollision() reports a distance of zero and hands back the
			// query point itself as the closest point, rather than any point on the surface.
			outConstraint.SurfacePlanePoint  = FVector::ZeroVector;    // Local space: the component's origin lies on its plane.
			outConstraint.SurfacePlaneNormal = FVector::ForwardVector; // Local space: the widget faces along its own +X axis.
		}
		break;

	case EWidgetGeometryMode::Cylinder:
		{
			// Curved widgets are constrained against a tangent plane resampled at the current contact point on every
			// tick. Because the constraint only ever displaces along its normal, and that displacement is radial to the
			// cylinder, clamping to this tangent plane is equivalent to clamping to the arc itself. The contact point is
			// the mouse position, which the engine has already placed on the arc for us: unlike the planar case, it does
			// not trust the collision query but re-derives the surface point from the widget's own geometry, so it stays
			// usable once this component has penetrated the widget. UpdateTouchResistance() runs after the trace within
			// the same tick, so it is never stale.
			//
			// Note that the tangent plane is only exactly correct at the point it was sampled from, whereas a planar
			// widget's plane stays correct for the lifetime of the constraint. For moving surfaces, this can result in
			// slightly stale constraint planes. However, the potential error is judged to be too small to warrant the
			// complexity of fixing it. Best practices would likely rule out movable cylindrical widgets anyway.
			//
			const FVector contactPoint  = GetMousePosition();
			const FVector contactNormal = WidgetUtils::GetWidgetNormalAtPosition(hoveredWidgetComponent, contactPoint);
			if (contactNormal.IsNearlyZero())
			{
				return false;
			}

			const FTransform& surfaceTransform = hoveredWidgetComponent->GetComponentTransform();
			outConstraint.SurfacePlanePoint    = surfaceTransform.InverseTransformPositionNoScale(contactPoint);
			outConstraint.SurfacePlaneNormal   = surfaceTransform.InverseTransformVectorNoScale(contactNormal);
		}
		break;

	default:
		// Unknown geometry: there is no surface here that we know how to clamp against, so decline rather than guess.
		ensureMsgf(false, TEXT("Touch resistance does not support this widget geometry mode."));
		return false;
	}

	// Hold the clamp a touch radius clear of the surface, so that this interactor's contact surface comes to rest against
	// the widget rather than its origin sinking into it. The plane is expressed in the surface component's local space,
	// which FSurfaceConstraint resolves without scale, so local units are world units here.
	outConstraint.SurfacePlanePoint += outConstraint.SurfacePlaneNormal.GetSafeNormal() * TouchRadius;

	return true;
}


void UTouchWidgetInteractionComponent::DrawDebugGeometry(const UWorld* world, const FWidgetInteractionDebugInfo& debugInfo) const
{
#if ENABLE_DRAW_DEBUG
	using namespace WidgetInteractionDebug;

	Super::DrawDebugGeometry(world, debugInfo);

	// Touch distance bands, marked off along the surface normal at the contact point so the interactor's position can be
	// read against the hover/select thresholds directly.
	if (!debugInfo.SurfaceNormal.IsNearlyZero())
	{
		DrawDebugLine(world, debugInfo.ContactPosition + debugInfo.SurfaceNormal * MinHoverDistance, debugInfo.ContactPosition + debugInfo.SurfaceNormal * MaxHoverDistance, HoverColor);
		DrawDebugCircle(world, debugInfo.ContactPosition + debugInfo.SurfaceNormal * MaxSelectDistance, BandRingRadius, CircleSegments, SelectColor, false, -1.f, 0, 0.f, debugInfo.SurfaceTangentY, debugInfo.SurfaceTangentZ, false);
		DrawDebugCircle(world, debugInfo.ContactPosition + debugInfo.SurfaceNormal * MinClearDistance, BandRingRadius, CircleSegments, CheckDeviationColor, false, -1.f, 0, 0.f, debugInfo.SurfaceTangentY, debugInfo.SurfaceTangentZ, false);
	}
#endif
}


void UTouchWidgetInteractionComponent::AppendDebugText(FString& outText) const
{
	using namespace WidgetInteractionDebug;
	
	Super::AppendDebugText(outText);

	// Insert the distance thresholds alongside the distance value that was appended by the base class.
	const FString thresholds = FString::Printf(TEXT("  (select<=%.2f  clear>%.2f  hover[%.2f..%.2f])"), MaxSelectDistance, MinClearDistance, MinHoverDistance, MaxHoverDistance);
	const int32 distanceLabelIndex = outText.Find(DistanceLabel);
	
	if (distanceLabelIndex != INDEX_NONE)
	{
		const int32 lineEnd = outText.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, distanceLabelIndex);
		outText.InsertAt(lineEnd != INDEX_NONE ? lineEnd : outText.Len(), thresholds);
	}
	else if (HoverDistance < TNumericLimits<float>::Max() * 0.5f) // Distances are left at FLT_MAX while nothing is hovered, which is not worth displaying.
	{
		outText += FString::Printf(TEXT("\n%s%.2f%s"), DistanceLabel, HoverDistance, *thresholds);
	}
	
	outText += FString::Printf(TEXT("\nresist:%d  can select:%d") , bIsTouchResisting, bCanSelect);
}
