// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs
{
	/**
	 * Creates a native delegate bound to the same UFunction as a dynamic (Blueprint) delegate, so that one native
	 * implementation can serve both. Returns an unbound delegate when the dynamic delegate is unbound.
	 * The signatures must match; the native delegate calls the UFunction by name.
	 *
	 *     DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDone, int32, value);
	 *     using FOnDoneNative = TDelegate<void(int32)>;
	 *     void Start(const FOnDone& onDone) { StartNative(Drs::AsNative<FOnDoneNative>(onDone)); }
	 */
	template <typename NativeDelegateType, typename DynamicDelegateType>
	NativeDelegateType AsNative(const DynamicDelegateType& dynamicDelegate)
	{
		// CreateUFunction asserts on a None function name, so an unbound delegate stays unbound.
		if (!dynamicDelegate.IsBound())
		{
			return NativeDelegateType();
		}
		return NativeDelegateType::CreateUFunction(const_cast<UObject*>(dynamicDelegate.GetUObject()), dynamicDelegate.GetFunctionName());
	}
}
