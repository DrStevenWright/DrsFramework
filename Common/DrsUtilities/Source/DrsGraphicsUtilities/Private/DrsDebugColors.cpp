// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsDebugColors.h"

FLinearColor Drs::Graphics::GetDebugColor(int32 Index)
{
	// Golden-ratio hue stepping keeps consecutive indices visually far apart.
	constexpr float GoldenRatioConjugate = 0.618033988749895f;
	const float Hue = FMath::Fractional(Index * GoldenRatioConjugate);
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue * 255.0f), 200, 255);
}
