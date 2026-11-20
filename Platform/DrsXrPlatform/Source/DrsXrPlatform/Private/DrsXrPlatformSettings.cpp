// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatform/DrsXrPlatformSettings.h"

#include "Misc/ConfigCacheIni.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrPlatformSettings)

EDrsXrHandTrackingSupport UDrsXrPlatformSettings::ParseHandTrackingSupport()
{
	// The names match the enum's, as DefaultEngine.ini and DrsXrPlatform_UPL.xml use them.
	FString value;
	if (GConfig && GConfig->GetString(TEXT("/Script/DrsXrPlatform.DrsXrPlatformSettings"), TEXT("HandTrackingSupport"), value, GEngineIni))
	{
		value.TrimStartAndEndInline();
		if (value == TEXT("ControllersOnly"))
		{
			return EDrsXrHandTrackingSupport::ControllersOnly;
		}
		if (value == TEXT("HandsOnly"))
		{
			return EDrsXrHandTrackingSupport::HandsOnly;
		}
	}
	return EDrsXrHandTrackingSupport::ControllersAndHands;
}
