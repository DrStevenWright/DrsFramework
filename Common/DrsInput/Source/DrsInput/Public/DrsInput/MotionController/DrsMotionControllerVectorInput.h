// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs::Input
{
	/**
	 * Per-frame state tracker for a 2D thumbstick. Set the vector as it arrives (SetVector/SetX/SetY), then call Update
	 * once per frame to refresh the derived magnitude and idle timing.
	 */
	struct DRSINPUT_API FMotionControllerVectorInput
	{
		void	Update(float deltaTime);
		void	Reset();
	
		void	SetX(float x)						{ Vector.X = x; }
		void	SetY(float y)						{ Vector.Y = y; }
		float	GetX() const						{ return Vector.X; }
		float	GetY() const						{ return Vector.Y; }
		void	SetVector(const FVector2D& vector)	{ Vector = vector; }
		FVector2D GetVector() const					{ return Vector; }
		float	GetDeflection() const				{ return Magnitude; }
	
		// The stick is idle while its deflection is at or below IdleThreshold.
		bool	IsIdle() const						{ return bIdle; }
		bool	IsJustIdle() const					{ return bIdle && IdleTime == 0.0f; }
		float	GetIdleTime() const					{ return bIdle ? IdleTime : 0.0f; }
	
	private:

		FVector2D	Vector		= FVector2D::ZeroVector;
		float		Magnitude	= 0.0f;
		bool		bIdle		= false;
		float		IdleTime	= 0.0f; // Time spent in the current idle state.

		static constexpr float	IdleThreshold = 0.1f;
	};
}
