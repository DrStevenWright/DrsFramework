// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "Utils/WidgetUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/DrsWidgetInteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	struct FWidgetArc
	{
		// Center of curvature - i.e. a point on the cylinder's axis, in component local space.
		// The arc curves away from the widget's +X face, so this sits in front of it and the surface normal points back towards it.
		FVector Center	= FVector::ZeroVector;
		float HalfAngle	= 0.0f; // Radians.
	};

	bool GetWidgetLocalArc(const UWidgetComponent* widgetComponent, FWidgetArc& outArc)
	{
		// This returns the widget's center of curvature - i.e. a point on the cylinder's central axis.
		if (widgetComponent == nullptr || !ensure(widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder))
		{
			return false;
		}

		const float arcAngle     = FMath::DegreesToRadians(widgetComponent->GetCylinderArcAngle());
		const float arcHalfAngle = 0.5f * arcAngle;
		if (FMath::IsNearlyZero(arcAngle))
		{
			return false; // Sanity check to avoid divide-by-zero.
		}
		
		// The arc length is simply the unwrapped width of the widget.
		// Multiply this length by the component scale in order to remain compatible with subsequent TransformPositionNoScale() operations.
		// Note: Widget co-ords are width (X) x height (Y), while the component scale components are depth (X) x right (Y) x up (Z).
		const float arcLength = widgetComponent->GetCurrentDrawSize().X * widgetComponent->GetComponentScale().Y;
		
		// The apothem is the perpendicular (+X) distance from the chord plane to the cylinder's central axis.
		const float arcRadius = arcLength / arcAngle;
		const float apothem = arcRadius * FMath::Cos(arcHalfAngle);                     
		
		// Pivots shift the widget in the chord plane. So for pivots with width component (X) != 0.5, we need to move the
		// central axis the same distance (in the chord direction) to compensate.
		const float chordLength = 2.0f * arcRadius * FMath::Sin(arcHalfAngle);
		const float pivotOffset = (widgetComponent->GetPivot().X - 0.5f) * chordLength; 
		
		outArc.Center    = FVector(apothem, pivotOffset, 0.0f);
		outArc.HalfAngle = arcHalfAngle;

		return true;
	}
}

UWorld* WidgetUtils::GetWorld(UObject* outer)
{
	/* The implementation below was copied and pasted from UWidget::GetWorld() in UE 5.2 */
	// UWidget's are given world scope by their owning user widget. We can get that through the widget tree that should
	// be the outer of this widget.
	if (UWidgetTree* owningTree = Cast<UWidgetTree>(outer))
	{
		return owningTree->GetWorld();
	}

	return nullptr;
}

FVector WidgetUtils::GetWidgetCenter(const UWidgetComponent* widgetComponent)
{
	check(widgetComponent);
	
	// For cylindrical widgets, get the center of the arc's curvature - a point on the cylinder's axis, rather than the
	// center of the widget surface itself.
	FWidgetArc arc;
	if (widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder && GetWidgetLocalArc(widgetComponent, arc))
	{
		return widgetComponent->GetComponentTransform().TransformPositionNoScale(arc.Center);
	}
	
	const FVector	widgetScale		= widgetComponent->GetComponentScale();
	const FVector2D widgetDrawSize	= widgetComponent->GetCurrentDrawSize();
	const FVector2D widgetPivot		= widgetComponent->GetPivot();
	
	if (!(widgetPivot - FVector2D(0.5f, 0.5f)).IsNearlyZero())
	{
		FVector	localCenter			= FVector::ZeroVector;
		localCenter.Y				= (widgetPivot.X - 0.5f) * widgetDrawSize.X * widgetScale.Y;
		localCenter.Z				= (widgetPivot.Y - 0.5f) * widgetDrawSize.Y * widgetScale.Z;
		return widgetComponent->GetComponentTransform().TransformPositionNoScale(localCenter);
	}
	
	return widgetComponent->GetComponentLocation();
}

FVector WidgetUtils::GetWidgetNormalAtPosition(const UWidgetComponent* widgetComponent, const FVector& position)
{
	// For a given widget component, this function returns a vector that is normal to the widget surface at some position.
	// Planar widgets have the same normal at all positions and this normal is defined by the widget component's X axis.
	// For cylindrical widgets, the normal is derived from the vector between the surface position and a parallel point on
	// the cylinder's up axis.
	
	if (widgetComponent == nullptr)
	{
		return FVector::ForwardVector;
	}
	else if (widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder)
	{
		const FVector arcCenter			= GetWidgetCenter(widgetComponent);
		const FVector cylinderUpAxis	= widgetComponent->GetUpVector();
		const FVector centerDirection	= arcCenter - position;
		const FVector normalDirection	= centerDirection - (cylinderUpAxis * (centerDirection | cylinderUpAxis));
	
		return normalDirection.GetSafeNormal();
	}
	else
	{
		// Unknown geometry is approximated as planar rather than declined: an unusable normal here would break the
		// interaction entirely, whereas a flat one merely misreports the surface's orientation.
		ensureMsgf(widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Plane, TEXT("Unsupported widget geometry mode; assuming the widget is planar."));
		return widgetComponent->GetForwardVector();
	}
}

bool WidgetUtils::IsPositionWithinWidgetInteractionBounds(const UWidgetComponent* widgetComponent, const FVector& position)
{
	// This function determines whether a 3D world space position is within the appropriate bounds to interact with a
	// given widget component.
	
	if (widgetComponent)
	{
		if (widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder)
		{
			return IsPositionWithinCylindricalWidgetBounds(widgetComponent, position);
		}
		else
		{
			// Unknown geometry is approximated as planar rather than declined: reporting every position as out of bounds
			// would silently disable interaction with the widget, whereas planar bounds remain roughly usable.
			ensureMsgf(widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Plane, TEXT("Unsupported widget geometry mode; assuming the widget is planar."));
			return IsPositionWithinPlanarWidgetBounds(widgetComponent, position);
		}
	}

	return false;
}

bool WidgetUtils::IsPositionWithinPlanarWidgetBounds(const UWidgetComponent* widgetComponent, const FVector& position)
{
	// This function determines whether a 3D world space position is within the bounds of a given widget component,
	// for which the widget surface is planar (not curved). (Note: This function also acts as a fallback for unrecognized
	// geometry types, therefore we do not validate the widget's geometry type here.)
	
	if (widgetComponent == nullptr)
	{
		return false;
	}
	
	const FVector localPosition		= widgetComponent->GetComponentTransform().InverseTransformPositionNoScale(position);
	const FVector widgetScale		= widgetComponent->GetComponentScale();
	const FVector2D widgetDrawSize	= widgetComponent->GetCurrentDrawSize();
	const FVector2D widgetPivot		= widgetComponent->GetPivot();
	const float widgetWidth			= widgetDrawSize.X * widgetScale.Y;
	const float widgetHeight		= widgetDrawSize.Y * widgetScale.Z;
	const float widthOffset			= widgetWidth * widgetPivot.X;
	const float heightOffset		= widgetHeight * widgetPivot.Y;
	
	return	(localPosition.Z >= (heightOffset - widgetHeight)) && (localPosition.Z <= heightOffset) &&
			(localPosition.Y >= (widthOffset - widgetWidth)) && (localPosition.Y <= widthOffset);
}

bool WidgetUtils::IsPositionWithinCylindricalWidgetBounds(const UWidgetComponent* widgetComponent, const FVector& position)
{
	// This function determines whether a 3D world space position is within the bounds of a given widget component,
	// for which the widget surface is cylindrical. For cylindrical widgets the width in 2D space corresponds to the
	// arc length in 3D space.
	
	if (widgetComponent == nullptr || !ensure(widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder))
	{
		return false;
	}
	
	const FVector localPosition		= widgetComponent->GetComponentTransform().InverseTransformPositionNoScale(position);
	const FVector widgetScale		= widgetComponent->GetComponentScale();
	const FVector2D widgetDrawSize	= widgetComponent->GetCurrentDrawSize();
	const FVector2D widgetPivot		= widgetComponent->GetPivot();
	
	const float widgetHeight		= widgetDrawSize.Y * widgetScale.Z;
	const float	heightOffset		= widgetHeight * widgetPivot.Y;

	if ((localPosition.Z < (heightOffset - widgetHeight)) || (localPosition.Z > heightOffset))
	{
		return false;
	}
	
	const float arcLength			= widgetDrawSize.X * widgetScale.Y;
	const float arcAngle			= FMath::DegreesToRadians(widgetComponent->GetCylinderArcAngle());
	const float arcRadius			= arcLength / arcAngle; // Because radians.
	const float widgetWidth			= 2.0f * arcRadius * FMath::Sin( 0.5f * arcAngle);
	const float	widthOffset			= widgetWidth * widgetPivot.X;

	if ((localPosition.Y < (widthOffset - widgetWidth)) || (localPosition.Y > widthOffset))
	{
		return false;
	}
	
	return true;
}

bool WidgetUtils::IsPositionWithinCylindricalWidgetAngleBounds(const UWidgetComponent* widgetComponent, const FVector& position)
{
	// This function determines whether a 3D world space position is within the height and angle bounds of a cylindrical
	// widget component.
	
	if (widgetComponent == nullptr || !ensure(widgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder))
	{
		return false;
	}
	
	const FVector localPosition		= widgetComponent->GetComponentTransform().InverseTransformPositionNoScale(position);
	const FVector widgetScale		= widgetComponent->GetComponentScale();
	const FVector2D widgetDrawSize	= widgetComponent->GetCurrentDrawSize();
	const FVector2D widgetPivot		= widgetComponent->GetPivot();
	const float widgetHeight		= widgetDrawSize.Y * widgetScale.Z;
	const float heightOffset		= widgetHeight * widgetPivot.Y;
	
	if (localPosition.Z < (heightOffset - widgetHeight) || (localPosition.Z > heightOffset))
	{
		return false;
	}

	FWidgetArc arc;
	if (!GetWidgetLocalArc(widgetComponent, arc))
	{
		return false;
	}

	FVector offsetFromCenter	= localPosition - arc.Center;
	offsetFromCenter.Z			= 0.0f;

	// The arc is symmetric about the axis-to-chord-midpoint direction, which is always the widget's local -X regardless
	// of where the pivot slides the axis along Y.
	return ((offsetFromCenter.GetSafeNormal() | -FVector::ForwardVector) >= FMath::Cos(arc.HalfAngle));
}

UDrsWidgetInteractionComponent* WidgetUtils::FindFocusedWidgetInteractionFromWidget(const UObject* worldContextObject, const UWidget* widget)
{
	const APawn* localPawn = UGameplayStatics::GetPlayerPawn(worldContextObject, 0);
	if (!localPawn || !widget)
	{
		return nullptr;
	}

	// We can return early if widget hasn't been focused at all.
	if (!widget->HasAnyUserFocus() && !widget->HasFocusedDescendants())
	{
		return nullptr;
	}
	
	const TInlineComponentArray<UDrsWidgetInteractionComponent*> widgetInteractionComponents(localPawn);
	for (UDrsWidgetInteractionComponent* widgetInteractionComponent : widgetInteractionComponents)
	{
		if (widgetInteractionComponent && widgetInteractionComponent->HasFocusedWidget(widget))
		{
			return widgetInteractionComponent;
		}
	}

	return nullptr;
}
