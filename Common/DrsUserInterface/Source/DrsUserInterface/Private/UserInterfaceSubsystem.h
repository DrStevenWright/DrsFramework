// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UserInterfaceSubsystem.generated.h"

class UDrsWidgetInteractionComponent;


/**
 * World Subsystem for managing UI interactions.
 */
UCLASS()
class DRSUSERINTERFACE_API UUserInterfaceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:

	// [UWorldSubsystem]
	virtual bool ShouldCreateSubsystem(UObject* outer) const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;
	virtual void OnWorldBeginPlay(UWorld& world) override;
	
	
public:

	static UUserInterfaceSubsystem* Get(const UObject* worldContext, EGetWorldErrorMode errorMode = EGetWorldErrorMode::ReturnNull);
	
	int32 ClaimVirtualUserIndex();
	void ReleaseVirtualUserIndex(int32 index);

	void RegisterWidgetInteractor(const UDrsWidgetInteractionComponent* widgetInteractor) { RegisteredWidgetInteractors.Add(widgetInteractor); }
	void UnregisterWidgetInteractor(const UDrsWidgetInteractionComponent* widgetInteractor) { RegisteredWidgetInteractors.Remove(widgetInteractor); }
	const TSet<const UDrsWidgetInteractionComponent*>& GetRegisteredWidgetInteractors() const { return RegisteredWidgetInteractors; }

private:
	
	// List of virtual user indices currently claimed by the widget interaction components.
	// This is static in order to avoid clashes in PIE multiplayer (under single process).
	static TArray<bool> VirtualUserIndices;

	UPROPERTY()
	TSet<const UDrsWidgetInteractionComponent*> RegisteredWidgetInteractors;
};
