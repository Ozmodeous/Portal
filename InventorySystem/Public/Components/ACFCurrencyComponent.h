// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include <GameplayTagContainer.h>

#include "ACFCurrencyComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrencyValueChanged, float, newValue, float, variation);


UCLASS(ClassGroup = (ACF), Blueprintable, meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFCurrencyComponent : public UActorComponent 
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UACFCurrencyComponent();

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void AddCurrency(float amount);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void RemoveCurrency(float amount);

    /*Override Current Currency value*/
    UFUNCTION(Server,  Reliable, BlueprintCallable, Category = ACF)
    void SetCurrency(float amount);

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE bool HasEnoughCurrency(float amount) const
    {
        return CurrencyAmount >= amount;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE float GetCurrentCurrencyAmount() const
    {
        return CurrencyAmount;
    }

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnCurrencyValueChanged OnCurrencyChanged;

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, ReplicatedUsing = OnRep_Currency, Category = ACF)
    float CurrencyAmount = 0.f;

    /*If set to true, if the entity owner is killed by a Player controlled character,
    all the currency will be added to that controller*/
    UPROPERTY(EditDefaultsOnly, Category = ACF)
    bool bDropCurrencyOnOwnerDeath = true;

    /*The randomic variation on the CurrencyAmount dropped on this owner death*/
    UPROPERTY(meta = (EditCondition = "bDropCurrencyOnOwnerDeath"), EditAnywhere, Category = ACF)
    float CurrencyDropVariation = 5.f;

    virtual void HandleCurrencyChanged();

private:
    UFUNCTION()
    void HandleStatReachedZero(FGameplayTag stat);

    UFUNCTION()
    void OnRep_Currency();

    void DispatchCurrencyChanged(float amount);
};
