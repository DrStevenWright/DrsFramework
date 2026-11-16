// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

/** Templated methods for returning an enum value as a string, omitting the enum class prefix. */
namespace Drs
{
	template<class EnumType>
	FString EnumToString(EnumType enumValue)
	{
		FString valueString = UEnum::GetValueAsString(enumValue);
		valueString.Split(TEXT("::"), nullptr, &valueString);
		return valueString;
	}

	template<class EnumType>
	FString EnumToString(TEnumAsByte<EnumType> enumValue)
	{
		FString valueString = UEnum::GetValueAsString(enumValue);
		valueString.Split(TEXT("::"), nullptr, &valueString);
		return valueString;
	}
}
