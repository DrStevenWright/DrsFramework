// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"

class UWidgetComponent;

/**
 * Shared debug information for widget interaction components.
 */
struct FWidgetInteractionDebugInfo
{
	// The widget component currently being hovered, if any. Null whenever nothing is hovered.
	const UWidgetComponent* WidgetComponent = nullptr;

	// World-space position of the interactor itself.
	FVector InteractorPosition = FVector::ZeroVector;

	// World-space contact point on the hovered widget, i.e. the virtual mouse position.
	FVector ContactPosition = FVector::ZeroVector;

	// Surface normal sampled at the contact point, plus two tangents spanning the surface there. Zero when nothing is
	// hovered, so check before using them to orient geometry.
	FVector SurfaceNormal = FVector::ZeroVector;
	FVector SurfaceTangentY = FVector::ZeroVector;
	FVector SurfaceTangentZ = FVector::ZeroVector;

	// Color representing the highest-precedence state the interactor is currently in.
	FColor StateColor = FColor::White;
};


/**
 * Shared presentation constants for widget interaction debug rendering.
 */
namespace WidgetInteractionDebug
{
	// Debug state listed in order of increasing precedence.
	const FColor SuspendedColor		= FColor(64, 64, 64);
	const FColor IdleColor			= FColor(140, 140, 140);
	const FColor HoverColor			= FColor::Cyan;
	const FColor SelectColor		= FColor::Green;
	const FColor DragColor			= FColor::Orange;
	const FColor AllowClickColor	= FColor::Green;
	const FColor DenyClickColor		= FColor::Red;
	const FColor CheckDeviationColor= FColor::Yellow;
	const FColor DeviationColor		= FColor::Magenta;
	const FColor TextColor			= FColor::White;
	
	// Debug text token that allows subclasses to insert distance thresholds into the existing debug text.
	inline const TCHAR* DistanceLabel = TEXT("dist:");

	constexpr float InteractorRadius	= 0.35f;
	constexpr float ContactRingRadius	= 1.0f;
	constexpr float BandRingRadius		= 0.6f;
	constexpr float PointSize			= 8.0f;
	constexpr float ForwardArrowLength	= 2.0f;
	constexpr int32 SphereSegments		= 8;
	constexpr int32 CircleSegments		= 24;
	const FVector TextOffset(0.0f, 0.0f, 6.0f);
}
