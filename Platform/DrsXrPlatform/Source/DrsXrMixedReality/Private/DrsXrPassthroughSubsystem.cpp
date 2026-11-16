// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrPassthroughSubsystem.h"
#include "DrsLog.h"
#include "DrsViewFadeLibrary.h"
#include "DrsXrPassthroughBackend.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrPassthroughSubsystem)

namespace
{
	const FName BlendRequestId(TEXT("(Drs) PassthroughBlend"));
	const FName ViewFadeSuspendReason(TEXT("(Drs) ViewFade"));
}

UDrsXrPassthroughSubsystem* UDrsXrPassthroughSubsystem::Get(const UObject* worldContext, EGetWorldErrorMode errorMode)
{
	const UWorld* world = GEngine ? GEngine->GetWorldFromContextObject(worldContext, errorMode) : nullptr;
	return world ? UGameInstance::GetSubsystem<UDrsXrPassthroughSubsystem>(world->GetGameInstance()) : nullptr;
}

void UDrsXrPassthroughSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	FWorldDelegates::OnWorldCleanup.AddUObject(this, &UDrsXrPassthroughSubsystem::OnWorldCleanup);
#if DRS_PASSTHROUGH_SUPPORTED
	UDrsViewFadeLibrary::OnViewFadeOpaqueChanged.AddUObject(this, &UDrsXrPassthroughSubsystem::OnViewFadeOpaqueChanged);
	OnViewFadeOpaqueChanged(UDrsViewFadeLibrary::IsViewFadeOpaque());
#endif
}

void UDrsXrPassthroughSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
#if DRS_PASSTHROUGH_SUPPORTED
	UDrsViewFadeLibrary::OnViewFadeOpaqueChanged.RemoveAll(this);
#endif

	if (Requests.Contains(BlendRequestId))
	{
		UDrsViewFadeLibrary::SetViewHoldoutOpacity(0.0f);
	}
	Requests.Reset();
	UpdatePassthroughState();

	Super::Deinitialize();
}

bool UDrsXrPassthroughSubsystem::IsPassthroughSupported() const
{
	const FDrsXrPassthroughBackend* backend = GetDrsXrPassthroughBackend();
	return backend && backend->IsSupported();
}

void UDrsXrPassthroughSubsystem::RequestPassthrough(const UObject* worldContext, FName requestId)
{
	if (requestId.IsNone())
	{
		DRS_LOG(LogDrsXrMixedReality, Warning, "Ignoring a passthrough request without an ID.");
		return;
	}

	DRS_CLOG(!Requests.Contains(requestId), LogDrsXrMixedReality, Log, "Passthrough requested: '%s'.", *requestId.ToString());
	const UWorld* world = GEngine ? GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	Requests.Add(requestId, world);
	UpdatePassthroughState();
}

void UDrsXrPassthroughSubsystem::ReleasePassthrough(FName requestId)
{
	if (Requests.Remove(requestId) > 0)
	{
		DRS_LOG(LogDrsXrMixedReality, Log, "Passthrough released: '%s'.", *requestId.ToString());
		UpdatePassthroughState();
	}
}

void UDrsXrPassthroughSubsystem::SuspendPassthrough(FName reason)
{
	if (reason.IsNone())
	{
		DRS_LOG(LogDrsXrMixedReality, Warning, "Ignoring a passthrough suspension without a reason.");
		return;
	}

	if (!Suspensions.WasVoteCast(reason))
	{
		DRS_LOG(LogDrsXrMixedReality, Log, "Passthrough suspended: '%s'.", *reason.ToString());
		Suspensions.CastVote(reason);
		UpdatePassthroughState();
	}
}

void UDrsXrPassthroughSubsystem::UnsuspendPassthrough(FName reason)
{
	if (Suspensions.WasVoteCast(reason))
	{
		DRS_LOG(LogDrsXrMixedReality, Log, "Passthrough unsuspended: '%s'.", *reason.ToString());
		Suspensions.WithdrawVote(reason);
		UpdatePassthroughState();
	}
}

void UDrsXrPassthroughSubsystem::SetPassthroughBlend(const UObject* worldContext, float amount)
{
	amount = FMath::Clamp(amount, 0.0f, 1.0f);
	UDrsViewFadeLibrary::SetViewHoldoutOpacity(amount);
	if (amount > 0.0f)
	{
		RequestPassthrough(worldContext, BlendRequestId);
	}
	else
	{
		ReleasePassthrough(BlendRequestId);
	}
}

float UDrsXrPassthroughSubsystem::GetPassthroughBlend() const
{
	return UDrsViewFadeLibrary::GetViewHoldoutOpacity();
}

void UDrsXrPassthroughSubsystem::UpdatePassthroughState()
{
	if (FDrsXrPassthroughBackend* backend = GetDrsXrPassthroughBackend())
	{
		backend->SetPassthroughVisible(IsPassthroughRequested() && !IsPassthroughSuspended());
	}
}

void UDrsXrPassthroughSubsystem::OnWorldCleanup(UWorld* world, bool hasSessionEnded, bool shouldCleanupResources)
{
	TArray<FName> releasedRequests;
	for (const TPair<FName, TWeakObjectPtr<const UWorld>>& request : Requests)
	{
		if (request.Value.Get() == world)
		{
			releasedRequests.Add(request.Key);
		}
	}

	for (const FName& requestId : releasedRequests)
	{
		if (requestId == BlendRequestId)
		{
			UDrsViewFadeLibrary::SetViewHoldoutOpacity(0.0f);
		}
		ReleasePassthrough(requestId);
	}
}

void UDrsXrPassthroughSubsystem::OnViewFadeOpaqueChanged(bool isOpaque)
{
	// Switch while the fade is opaque, where the switch is invisible. Level transitions fade to opaque before the loading
	// screen; after that the scene is no longer submitted, so a see-through background would show passthrough behind the
	// loading screen. Switching early also covers runtimes that take a few frames to apply a blend mode (seen on Meta).
	if (isOpaque)
	{
		SuspendPassthrough(ViewFadeSuspendReason);
	}
	else
	{
		UnsuspendPassthrough(ViewFadeSuspendReason);
	}
}
