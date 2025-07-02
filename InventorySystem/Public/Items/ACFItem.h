// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFItemTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>
#include <AbilitySystemComponent.h>

#include "ACFItem.generated.h"

class UGameplayEffect;
struct FGameplayEffectSpecHandle;

USTRUCT(BlueprintType)
struct FBaseItem : public FTableRowBase {
    GENERATED_BODY()

public:
    FBaseItem() { ItemGuid = FGuid::NewGuid(); };

    FBaseItem(const TSubclassOf<class AACFItem>& inItem, const FGuid forcedGuid, int32 inCount)
    {
        ItemGuid = forcedGuid;
        ItemClass = inItem;
        Count = inCount;
    };

    FBaseItem(const TSubclassOf<class AACFItem>& inItem, int32 inCount)
    {
        ItemGuid = FGuid::NewGuid();
        ItemClass = inItem;
        Count = inCount;
    };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = ACF)
    TSubclassOf<class AACFItem> ItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = ACF)
    int32 Count = 1;

    FORCEINLINE bool operator==(const FBaseItem& Other) const
    {
        return this->ItemClass == Other.ItemClass;
    }

    FORCEINLINE bool operator!=(const FBaseItem& Other) const
    {
        return this->ItemClass != Other.ItemClass;
    }

    FORCEINLINE bool operator!=(const TSubclassOf<class AACFItem>& Other) const
    {
        return this->ItemClass != Other;
    }

    FORCEINLINE bool operator==(const TSubclassOf<class AACFItem>& Other) const
    {
        return this->ItemClass == Other;
    }

protected:
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF)
    FGuid ItemGuid;
};

USTRUCT(BlueprintType)
struct FItemDescriptor : public FTableRowBase {
    GENERATED_BODY()

public:
    FItemDescriptor()
    {
        ThumbNail = nullptr;
        WorldMesh = nullptr;
        GameSpecificData = nullptr;
        ItemType = EItemType::Other;
        Scale = FVector2D(1.f, 1.f);
    };

    /*Icon to be displayed in UI*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF|Icon")
    class UTexture2D* ThumbNail;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF|Icon")
    FVector2D Scale;

    /*Name of the item*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    FText Name;

    /*Long description of the item*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    FText Description;

    /*Type of item*/
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    EItemType ItemType;

    /*Decide the amount of stackable units when this item is in inventory*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    uint8 MaxInventoryStack = 1;

    /*Weight of this item in the inventory*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    float ItemWeight = 5.0;

    /*mesh to be used when spawned in world as world mesh, not mandatory*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    UStaticMesh* WorldMesh;

    /* If the item is droppable in the world*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    bool bDroppable = true;

    /* If this item can be upgraded*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    bool bUpgradable = false;

    /*If upgrading this item has a cost in currency*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bUpgradable == true"), Category = ACF)
    float UpgradeCurrencyCost = 0.f;

    /* The items required to upgrade this item*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bUpgradable == true"), Category = ACF)
    TArray<FBaseItem> RequiredItemsToUpgrade;

    /* Next level of this item*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bUpgradable == true"), Category = ACF)
    TSubclassOf<AACFItem> NextLevelClass;

    /* If this item can be sold to or from a Vendor*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    bool bSellable = true;

    /*base price for selling / buying this item*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bSellable == true"), Category = ACF)
    float CurrencyValue = 5.0;

    /*A Tag to identify the rarity of the item*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    FGameplayTag Rarity;

    /*Slots in which this item can be equipped*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    TArray<FGameplayTag> ItemSlots;

    TArray<FGameplayTag> GetPossibleItemSlots() const { return ItemSlots; }

    /*Usable to add game specific data to any item*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    UPrimaryDataAsset* GameSpecificData;
};

UCLASS()
class INVENTORYSYSTEM_API AACFItem : public AActor {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AACFItem();

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UTexture2D* GetThumbnailImage() const { return ItemInfo.ThumbNail; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FText GetItemName() const { return ItemInfo.Name; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FText GetItemDescription() const { return ItemInfo.Description; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE EItemType GetItemType() const { return ItemInfo.ItemType; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class APawn* GetItemOwner() const { return ItemOwner; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FItemDescriptor GetItemInfo() const { return ItemInfo; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE TArray<FGameplayTag> GetPossibleItemSlots() const { return ItemInfo.GetPossibleItemSlots(); }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetItemDescriptor(const FItemDescriptor& itemDesc)
    {
        ItemInfo = itemDesc;
    }

    void SetItemOwner(APawn* inOwner)
    {
        ItemOwner = inOwner;
    }

private:
protected:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ItemOwner, Category = ACF)
    class APawn* ItemOwner;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | Item")
    FItemDescriptor ItemInfo;

    UFUNCTION()
    virtual void OnRep_ItemOwner();


    FActiveGameplayEffectHandle AddGASModifierToOwner(const TSubclassOf<UGameplayEffect>& gameplayModifier );
	void RemoveGASModifierToOwner(const FActiveGameplayEffectHandle& modifierHandle);
    UAbilitySystemComponent* GetAbilityComponent() const;


    //     UFUNCTION(BlueprintCallable, CallInEditor)
    //     void AutoGenerateThumbnail();
};
