// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPlatformSubsystem.h"
#include "DrsLog.h"
#include "DrsXrPlatform.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrPlatformSubsystem)

void UDrsXrPlatformSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	DRS_LOG(LogDrsXrPlatform, Log, "XR system: %s", *GetXrSystemName().ToString());
}

bool UDrsXrPlatformSubsystem::IsXrSystemActive() const
{
	return GEngine && GEngine->XRSystem.IsValid();
}

FName UDrsXrPlatformSubsystem::GetXrSystemName() const
{
	return IsXrSystemActive() ? GEngine->XRSystem->GetSystemName() : NAME_None;
}

bool UDrsXrPlatformSubsystem::SetFloorTrackingOrigin()
{
	if (!IsXrSystemActive())
	{
		return false;
	}
	GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
	DRS_LOG(LogDrsXrPlatform, Log, "Requested LocalFloor tracking origin; active origin is %d.", static_cast<int32>(GEngine->XRSystem->GetTrackingOrigin()));
	return true;
}
