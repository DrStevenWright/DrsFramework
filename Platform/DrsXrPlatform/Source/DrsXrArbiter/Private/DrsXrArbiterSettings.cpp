// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiterSettings.h"

#include "DrsXrArbiter/DrsXrArbiter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrArbiterSettings)

TArray<FString> UDrsXrArbiterSettings::GetXrPluginOptions()
{
	TArray<FString> options = { TEXT("None") };
	for (const FDrsXrVendor& vendor : Drs::XrArbiter::GetVendors())
	{
		options.Add(vendor.Id.ToString());
	}
	return options;
}
