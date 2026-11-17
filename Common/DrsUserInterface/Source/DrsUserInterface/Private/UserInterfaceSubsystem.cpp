// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "UserInterfaceSubsystem.h"

#include "CoreUserInterfaceSettings.h"

TArray<bool> UUserInterfaceSubsystem::VirtualUserIndices;


UUserInterfaceSubsystem* UUserInterfaceSubsystem::Get(const UObject* worldContext, EGetWorldErrorMode errorMode)
{
	return UWorld::GetSubsystem<UUserInterfaceSubsystem>(GEngine->GetWorldFromContextObject(worldContext, errorMode));
}


bool UUserInterfaceSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
	if (!Super::ShouldCreateSubsystem(outer))
	{
		return false;
	}
	
	const UWorld* world = Cast<UWorld>(outer);
	return world && !world->IsNetMode(NM_DedicatedServer) && !FPackageName::IsTempPackage(world->GetPackage()->GetName());
}


bool UUserInterfaceSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
	return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}


void UUserInterfaceSubsystem::OnWorldBeginPlay(UWorld& world)
{
	Super::OnWorldBeginPlay(world);

	// Apply the global UI input scroll speed setting based on the current mode (VR or Non-VR).
	UCoreUserInterfaceSettings::GetCoreUserInterfaceSettings()->ApplyInputScrollSpeedSetting();
}


int32 UUserInterfaceSubsystem::ClaimVirtualUserIndex()
{
	for (int32 i = 0; i < VirtualUserIndices.Num(); ++i)
	{
		if (!VirtualUserIndices[i])
		{
			VirtualUserIndices[i] = true;
			return i;
		}
	}
	
	VirtualUserIndices.Add(true);
	return VirtualUserIndices.Num() - 1;
}


void UUserInterfaceSubsystem::ReleaseVirtualUserIndex(int32 index)
{
	if (VirtualUserIndices.IsValidIndex(index))
	{
		VirtualUserIndices[index] = false;
	}
}
