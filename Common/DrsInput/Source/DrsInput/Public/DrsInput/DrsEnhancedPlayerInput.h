// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "EnhancedPlayerInput.h"

#include "DrsEnhancedPlayerInput.generated.h"

class UInputMappingContext;

/**
 * Custom version of UEnhancedPlayerInput required to access protected members.
 */
UCLASS(config = Input, transient)
class DRSINPUT_API UDrsEnhancedPlayerInput : public UEnhancedPlayerInput
{
	GENERATED_BODY()

public:

	void ReleaseDanglingInputs(const UInputMappingContext* contextBeingRemoved) const;
	void ReleaseOverriddenInputs(const UInputMappingContext* overridingContext, int32 priority);
	
};
