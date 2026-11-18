// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Subsystems/LocalPlayerSubsystem.h"
#include "UObject/ObjectKey.h"
#include "DrsInputContextSubsystem.generated.h"

class UInputMappingContext;

/**
 * Remembers, for one local player, why each input mapping context was added. A context is only removed once every reason
 * for it has been withdrawn. The reasons belong to the local player, so they are never shared between players or between
 * play sessions, and they don't depend on any pawn or input component.
 */
UCLASS()
class DRSINPUT_API UDrsInputContextSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:

	/** Records a reason for the context to be added. */
	void AddReason(const UInputMappingContext* context, FName reason);

	/** Withdraws a reason. Returns true if the context has no reasons left, or never had any. */
	bool RemoveReason(const UInputMappingContext* context, FName reason);

	bool HasReason(const UInputMappingContext* context, FName reason) const;

	TArray<FName> GetReasons(const UInputMappingContext* context) const;
	FString GetReasonsString(const UInputMappingContext* context) const;
	void PrintReasons(const UInputMappingContext* context) const;

private:

	// Keyed by the context object, not its name, so that two contexts with the same name in different folders stay apart.
	TMap<TObjectKey<const UInputMappingContext>, TSet<FName>> ActiveInputContexts;
};
