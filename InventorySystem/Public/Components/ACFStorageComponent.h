// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFItemTypes.h"
#include "Components/ACFCurrencyComponent.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Items/ACFItem.h"

#include "ACFStorageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemsChanged, const TArray<FBaseItem>&, currentItems);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStorageEmpty);

/** 
 * Storage component that manages items and currency within an inventory system.
 */
UCLASS(ClassGroup = (ACF), Blueprintable, meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFStorageComponent : public UACFCurrencyComponent {
    GENERATED_BODY()

public:
    /** Sets default values for this component's properties */
    UACFStorageComponent();

protected:
    /** Called when the game starts */
    virtual void BeginPlay() override;

    /** The array of stored items */
    UPROPERTY(SaveGame, EditAnywhere, ReplicatedUsing = OnRep_Items, Category = ACF)
    TArray<FBaseItem> Items;

    /** Called when the component is loaded */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnComponentLoaded();

    /** Called when the component is saved */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnComponentSaved();

public:
    /** Removes multiple items from storage */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void RemoveItems(const TArray<FBaseItem>& inItems);

    /** Removes a single item from storage */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void RemoveItem(const FBaseItem& inItems);

    /** Adds a single item to storage */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void AddItem(const FBaseItem& inItems);

    /** Adds multiple items to storage */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void AddItems(const TArray<FBaseItem>& inItems);

    /** Moves items from storage to an equipment component */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void MoveItemsToInventory(const TArray<FBaseItem>& inItems, UACFEquipmentComponent* equipComp);

    /** Transfers currency to another currency component */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void GatherCurrency(float amount, UACFCurrencyComponent* currencyComp);

    /** Checks if a pawn can gather a specific item */
    UFUNCTION(BlueprintPure, Category = "ACF | Checks")
    bool CanPawnGatherItems(const APawn* pawn, const FBaseItem& itemsToGather);

    /** Transfers all currency to another currency component */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void GatherAllCurrency(UACFCurrencyComponent* currencyComp);

    /** Retrieves the currency component of a given pawn */
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    class UACFCurrencyComponent* GetPawnCurrencyComponent(const APawn* pawn) const;

    /** Retrieves the current currency amount of a given pawn */
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    float GetPawnCurrency(const APawn* pawn) const;

    /** Retrieves the equipment component of a given pawn */
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    class UACFEquipmentComponent* GetPawnEquipment(const APawn* pawn) const;

    /** Retrieves the inventory items of a given pawn */
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    TArray<FInventoryItem> GetPawnInventory(const APawn* pawn) const;

    /** Returns the list of stored items */
    UFUNCTION(BlueprintPure, Category = ACF)
    TArray<FBaseItem> GetItems() const { return Items; }

    /** Checks if the storage is empty */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool IsStorageEmpty();

    /** Event triggered when items change */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemsChanged OnItemChanged;

    /** Event triggered when storage becomes empty */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnStorageEmpty OnStorageEmpty;

    /** Handles currency changes in storage */
    virtual void HandleCurrencyChanged() override;

private:
    /** Replication function for item updates */
    UFUNCTION()
    void OnRep_Items();

    /** Checks if storage is empty and triggers appropriate events */
    void CheckEmpty();
};
