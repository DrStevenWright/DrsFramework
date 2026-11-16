// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsDebugColors.h"

FLinearColor Drs::Graphics::GetDebugColor(int32 index)
{
	// Golden-ratio hue stepping keeps consecutive indices visually far apart.
	constexpr float goldenRatioConjugate = 0.618033988749895f;
	const float hue = FMath::Fractional(index * goldenRatioConjugate);
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(hue * 255.0f), 200, 255);
}
