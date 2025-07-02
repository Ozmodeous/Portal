// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFItem.h"
#include "ACMTypes.h"
#include "ARSTypes.h"
#include "CoreMinimal.h"
#include <GameplayTagContainer.h>

#include "ACFConsumable.generated.h"

class UGameplayEffect;

/**
 *
 */

UCLASS()
class INVENTORYSYSTEM_API AACFConsumable : public AACFItem {
    GENERATED_BODY()

    friend class UACFEquipmentComponent;

public:
    UFUNCTION(BlueprintPure, Category = ACF)
    TArray<FStatisticValue> GetStatsModifiers() const { return StatModifier; }

    UFUNCTION(BlueprintPure, Category = ACF)
    TArray<FTimedAttributeSetModifier> GetTimedModifiers() const { return TimedAttributeSetModifier; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FGameplayTag GetDesiredUseAction() const { return DesiredUseAction; }

    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = ACF)
    bool CanBeUsed(const APawn* pawn);

    AACFConsumable();

protected:
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnItemUsed();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF | Consumable|ARS")
    TArray<FStatisticValue> StatModifier;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF | Consumable|ARS")
    TArray<FTimedAttributeSetModifier> TimedAttributeSetModifier;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF | Consumable|GAS")
    TSubclassOf<UGameplayEffect> ConsumableGameplayEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF | Consumable")
    FActionEffect OnUsedEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF | Consumable")
    FGameplayTag DesiredUseAction;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ACF | Consumable")
    bool bConsumeOnUse = true;

private:
    void Internal_UseItem(class ACharacter* target);
};
