// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ARSTypes.h"
#include "Components/CapsuleComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACFWorldItem.h"

#include "ACFPickup.generated.h"

UCLASS()
class INVENTORYSYSTEM_API AACFPickup : public AACFWorldItem {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AACFPickup();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = ACF)
    bool bPickOnOverlap = true;

    UPROPERTY(EditAnywhere, Category = ACF)
    bool bAutoEquipOnPick = true;

    UPROPERTY(VisibleAnywhere, Category = ACF)
    TObjectPtr<UCapsuleComponent> PickUpCapsule;

    UPROPERTY(EditAnywhere, Category = ACF)
    TArray<FStatisticValue> OnPickupEffect;

    UPROPERTY(EditAnywhere, Category = ACF)
    TArray<FTimedAttributeSetModifier> OnPickupBuff;

    // INTERACTION INTERFACE
    /* called when player interact with object of this class */
    virtual void OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;
    virtual void OnInteractableRegisteredByPawn_Implementation(class APawn* Pawn) override;
    // END INTERFACE
public:
};
