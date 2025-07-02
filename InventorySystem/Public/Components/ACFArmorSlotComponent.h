// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/SkeletalMeshComponent.h"
#include "CoreMinimal.h"
#include <Engine/SkeletalMesh.h>
#include <Engine/SkinnedAsset.h>
#include <GameplayTagContainer.h>

#include "ACFArmorSlotComponent.generated.h"

/**
 *
 */
UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFArmorSlotComponent : public USkeletalMeshComponent {
    GENERATED_BODY()

public:
    UACFArmorSlotComponent();

    /*Sets the mesh that needs to be used when this slot is empty.*/
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetEmptySlotMesh(USkinnedAsset* inEmptySlotMesh)
    {
        EmptySlotMesh = inEmptySlotMesh;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    USkinnedAsset* GetEmptySlotMesh() const
    {
        return EmptySlotMesh;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetSlotTag(FGameplayTag inSlot)
    {
        ArmorSlot = inSlot;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    FGameplayTag GetSlotTag() const
    {
        return ArmorSlot;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ResetSlotToEmpty();

protected:
    UPROPERTY(EditAnywhere, Category = ACF)
    FGameplayTag ArmorSlot;

    virtual void BeginPlay() override;

    virtual void InitializeComponent() override;

private:
    TObjectPtr<USkinnedAsset> EmptySlotMesh;
    bool hasEmptyVersion = false;
};
