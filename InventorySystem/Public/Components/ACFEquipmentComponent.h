// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include <Components/SkeletalMeshComponent.h>
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>

#include "ACFItemTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Items/ACFItem.h"

#include "ACFEquipmentComponent.generated.h"

class USkeletalMeshComponent;
class AACFConsumable;

USTRUCT(BlueprintType)
struct FStartingItem : public FBaseItem {
    GENERATED_BODY()

public:
    FStartingItem() { };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    bool bAutoEquip = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"), Category = ACF)
    float DropChancePercentage = 0.f;

    FORCEINLINE bool operator==(const FStartingItem& Other) const
    {
        return this->ItemClass == Other.ItemClass;
    }

    FORCEINLINE bool operator!=(const FStartingItem& Other) const
    {
        return this->ItemClass != Other.ItemClass;
    }
};

USTRUCT(BlueprintType)
struct FInventoryItem : public FBaseItem {
    GENERATED_BODY()

public:
    FInventoryItem() { };

    FInventoryItem(const FBaseItem& inItem);

    FInventoryItem(const FStartingItem& inItem);

    /*Info of the item in this inventory slot*/
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    FItemDescriptor ItemInfo;

    /*Can be set for grid  based inventories.
     NOT SET BY DEFAULT!!*/
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF)
    int32 InventoryIndex = -1;

    /*Identifies if this item is equipped*/
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF)
    bool bIsEquipped = false;

    /*If this item is equipped, this is the slot in which is equipped.
    Not set if the item is not equipped*/
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF)
    FGameplayTag EquipmentSlot;

    /*Chance of this item of being dropped on death*/
    UPROPERTY(SaveGame, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"), Category = ACF)
    float DropChancePercentage = 0.f;

    FGuid GetItemGuid() const
    {
        return ItemGuid;
    }

    void ForceGuid(const FGuid& newGuid)
    {
        ItemGuid = newGuid;
    }

    void RefreshDescriptor();

    FORCEINLINE bool operator==(const FInventoryItem& Other) const
    {
        return this->GetItemGuid() == Other.GetItemGuid();
    }

    FORCEINLINE bool operator!=(const FInventoryItem& Other) const
    {
        return this->GetItemGuid() != Other.GetItemGuid();
    }

    FORCEINLINE bool operator==(const FGuid& Other) const
    {
        return this->GetItemGuid() == Other;
    }

    FORCEINLINE bool operator!=(const FGuid& Other) const
    {
        return this->GetItemGuid() != Other;
    }
};

USTRUCT(BlueprintType)
struct FEquippedItem {
    GENERATED_BODY()

public:
    FEquippedItem()
    {
        Item = nullptr;
    };

    FEquippedItem(const FInventoryItem& item, const FGameplayTag& itemSlot, AACFItem* itemPtr)
    {
        ItemSlot = itemSlot;
        InventoryItem = item;
        InventoryItem.ForceGuid(item.GetItemGuid());
        InventoryItem.bIsEquipped = true;
        InventoryItem.EquipmentSlot = ItemSlot;
        Item = itemPtr;
    }

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FGameplayTag ItemSlot;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    FInventoryItem InventoryItem;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class AACFItem* Item;

    FGameplayTag GetItemSlot() const
    {
        return ItemSlot;
    }

    FORCEINLINE bool operator==(const FEquippedItem& Other) const
    {
        return this->ItemSlot == Other.ItemSlot;
    }

    FORCEINLINE bool operator!=(const FEquippedItem& Other) const
    {
        return this->ItemSlot != Other.ItemSlot;
    }

    FORCEINLINE bool operator==(const FGameplayTag& Other) const
    {
        return this->ItemSlot == Other;
    }

    FORCEINLINE bool operator!=(const FGameplayTag& Other) const
    {
        return this->ItemSlot != Other;
    }

    FORCEINLINE bool operator==(const FGuid& Other) const
    {
        return this->InventoryItem.GetItemGuid() == Other;
    }

    FORCEINLINE bool operator!=(const FGuid& Other) const
    {
        return this->InventoryItem.GetItemGuid() != Other;
    }
};

USTRUCT(BlueprintType)
struct FEquipment {
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class AACFWeapon* MainWeapon;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class AACFWeapon* SecondaryWeapon;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    TArray<FEquippedItem> EquippedItems;

    FEquipment()
    {
        MainWeapon = nullptr;
        SecondaryWeapon = nullptr;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentChanged, const FEquipment&, Equipment);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedArmorChanged, const FGameplayTag&, ArmorSlot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, const TArray<FInventoryItem>&, Inventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, const FBaseItem&, item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, const FBaseItem&, item);

UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFEquipmentComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UACFEquipmentComponent();

    /* Use this only on Server!!!
     *
     *Initialize a character's inventory and equipment, receives in input the mesh to be
     * used for equipment and attachments
     */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void InitializeStartingItems();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnEntityOwnerDeath();

    /*------------------------ INVENTORY
     * -----------------------------------------*/

      /*Adds the provided item to this Inventory */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void AddItemToInventoryByClass(TSubclassOf<AACFItem> inItem, int32 count = 1, bool bAutoEquip = true);

    /*Adds the provided item to this Inventory */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void AddItemToInventory(const FBaseItem& ItemToAdd, bool bAutoEquip = true);

     /*Adds the provided item to this Inventory and try to put it in to a specific index*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void AddItemToInventoryAtIndex(const FBaseItem& ItemToAdd, bool bAutoEquip = true, float dropChance = 0.f, const FGameplayTag& equipSlot = FGameplayTag(), int32 index = -1);

    /*Moves the item at the provided from the Inventory to the Equipment*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void UseInventoryItemByIndex(int32 index);

    /*Moves the item at the provided from the Inventory to the Equipment*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void UseInventoryItem(const FInventoryItem& item);

    /*Removes the selected amount of the provided item from the inventory and
       Unequips it if it is equipped*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void RemoveItem(const FInventoryItem& item, int32 count = 1);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void RemoveItemByIndex(const int32 index, int32 count = 1);

    /*Removes the selected amount of the provided item from the inventory,
       Unequips it if it is equipped and spawn a WorldItem near the owner position
       containing the selectede items*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void DropItem(const FInventoryItem& item, int32 count = 1);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void DropItemByInventoryIndex(int32 itemIndex, int32 count);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void ConsumeItems(const TArray<FBaseItem>& ItemsToCheck);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void SetInventoryItemSlotIndex(const FInventoryItem& item, int32 newIndex);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void SplitStackOfItems(const FInventoryItem& item, int32 count);

    UFUNCTION(BlueprintCallable, Category = "ACF | Inventory")
    void UseConsumableOnTarget(const FInventoryItem& Inventoryitem, ACharacter* target);

    UFUNCTION(BlueprintCallable, Category = "ACF | Inventory")
    bool CanUseConsumable(const FInventoryItem& Inventoryitem);
    /*------------------------ STORAGE -----------------------------------------*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void MoveItemsToInventory(const TArray<FBaseItem>& inItems, UACFStorageComponent* equipComp);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void MoveItemFromEquipment(UACFEquipmentComponent* sourceEquipmentComponent, const FInventoryItem& itemToMove, int32 count = 1, int32 inventoryIndex = -1, bool bAutoEquip = false, FGameplayTag equipSlot = FGameplayTag());

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void BuyItemFromEquipment(UACFEquipmentComponent* sourceEquipmentComponent, class UACFCurrencyComponent* sourceCurrencyComp, class UACFCurrencyComponent* targetCurrencyComp, const FInventoryItem& itemToBuy, int32 count = 1, int32 inventoryIndex = -1, float priceMultiplier = 1.f, bool bAutoEquip = false, FGameplayTag equipSlot = FGameplayTag());

    /*------------------------ EQUIPMENT
     * -----------------------------------------*/

    /*Moves the Inventory Item to the Equipment*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void EquipItemFromInventory(const FInventoryItem& inItem);

    /*Moves the Inventory Item to the Equipment*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void EquipItemFromInventoryInSlot(const FInventoryItem& inItem, FGameplayTag slot);

    /* Use the item in the specified slot of the Equipment*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UseEquippedItemBySlot(FGameplayTag itemSlot);

    /* Use the item in the specified slot of the Equipment*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UseConsumableOnActorBySlot(FGameplayTag itemSlot, ACharacter* target);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UnequipItemBySlot(FGameplayTag itemSlot);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UnequipItemByGuid(const FGuid& itemGuid);

    /*Removes the currently weapon in the hand of the character*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void SheathCurrentWeapon();

    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetDamageActivation(bool isActive, const TArray<FName>& traceChannels, bool isLeftWeapon = false);

    /*Overrides default character mesh with the provided one*/
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetMainMesh(USkeletalMeshComponent* newMesh, bool bRefreshEquipment = true);

    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    USkeletalMeshComponent* GetMainMesh() const
    {
        return MainCharacterMesh;
    }

    UFUNCTION(Server, Reliable, Category = "ACF | Equipment")
    void DestroyEquippedItems();

    /* Refresh the appearence of the owner based on current Equipment. USEFULL FOR
     * LATE JOINING CONSISTENCY!*/
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void RefreshEquipment();

    UFUNCTION(BlueprintCallable, Category = ACF)
    void RefreshTotalWeight();

    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    bool ShouldUseLeftHandIK() const;

    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    FVector GetLeftHandIkPos() const;

    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    bool IsSlotAvailable(const FGameplayTag& itemSlot) const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    bool TryFindAvailableItemSlot(const TArray<FGameplayTag>& itemSlots, FGameplayTag& outAvailableSlot);

    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    bool HaveAtLeastAValidSlot(const TArray<FGameplayTag>& itemSlots);

    /*------------------------ SETTERS -----------------------------------------*/

    UFUNCTION(BlueprintCallable, Category = "ACF | Setters")
    void SetMaxInventoryWeight(int32 newMax)
    {
        MaxInventoryWeight = newMax;
    }

    UFUNCTION(BlueprintCallable, Category = "ACF | Setters")
    void SetMaxInventorySlots(int32 newMax)
    {
        MaxInventorySlots = newMax;
    }

    UFUNCTION(BlueprintCallable, Category = "ACF | Setters")
    void SetDropItemsOnDeath(bool bDrop)
    {
        bDropItemsOnDeath = bDrop;
    }

    /*------------------------ GETTERS -----------------------------------------*/

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE int32 GetMaxInventorySlots() const
    {
        return MaxInventorySlots;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE int32 GetMaxInventoryWeight() const
    {
        return MaxInventoryWeight;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE AACFWeapon* GetCurrentMainWeapon() const
    {
        return Equipment.MainWeapon;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE AACFWeapon* GetCurrentOffhandWeapon() const
    {
        return Equipment.SecondaryWeapon;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE FEquipment GetCurrentEquipment() const
    {
        return Equipment;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE TArray<FInventoryItem> GetInventory() const
    {
        return Inventory;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FVector GetMainWeaponSocketLocation() const;

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool IsInInventory(const FInventoryItem& item) const
    {
        return Inventory.Contains(item);
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    bool GetItemByGuid(const FGuid& itemGuid, FInventoryItem& outItem) const;

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool GetItemByIndex(const int32 index, FInventoryItem& outItem) const
    {
        if (Inventory.IsValidIndex(index)) {
            outItem = Inventory[index];
            return true;
        }
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE int32 GetFirstEmptyInventoryIndex() const
    {
        for (int32 i = 0; i < MaxInventorySlots; i++) {
            if (IsSlotEmpty(i))
                return i;
        }
        return -1;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool GetItemByInventoryIndex(const int32 index, FInventoryItem& outItem) const
    {
        for (const FInventoryItem& item : Inventory) {
            if (item.InventoryIndex == index) {
                outItem = item;
                return true;
            }
        }
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool IsSlotEmpty(int32 index) const
    {
        for (const FInventoryItem& item : Inventory) {
            if (item.InventoryIndex == index)
                return false;
        }
        return true;
    }

    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    int32 GetTotalCountOfItemsByClass(const TSubclassOf<AACFItem>& itemClass) const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    void GetAllItemsOfClassInInventory(const TSubclassOf<AACFItem>& itemClass, TArray<FInventoryItem>& outItems) const;

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    void GetAllSellableItemsInInventory(TArray<FInventoryItem>& outItems) const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool FindFirstItemOfClassInInventory(const TSubclassOf<AACFItem>& itemClass, FInventoryItem& outItem) const;

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE float GetCurrentInventoryTotalWeight() const
    {
        return currentInventoryWeight;
    }

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE TArray<FModularPart> GetModularMeshes() const
    {
        return ModularMeshes;
    }

    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool GetEquippedItemSlot(const FGameplayTag& itemSlot, FEquippedItem& outSlot) const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool GetEquippedItem(const FGuid& itemGuid, FEquippedItem& outSlot) const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool GetModularMesh(FGameplayTag itemSlot, FModularPart& outMesh) const;

    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    TArray<FStartingItem> GetStartingItems() const
    {
        return StartingItems;
    }

    /*------------------------ CHECKS -----------------------------------------*/

    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool HasAnyItemInEquipmentSlot(FGameplayTag itemSlor) const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    int32 NumberOfItemCanTake(const TSubclassOf<AACFItem>& itemToCheck);

    UFUNCTION(BlueprintPure, Category = "ACF | Checks")
    bool CanSwitchToRanged();

    UFUNCTION(BlueprintPure, Category = "ACF | Checks")
    bool CanSwitchToMelee();

    UFUNCTION(BlueprintPure, Category = "ACF | Checks")
    bool CanBeEquipped(const TSubclassOf<AACFItem>& equippable);

    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool HasOnBodyAnyWeaponOfType(TSubclassOf<AACFWeapon> weaponClass) const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool HasEnoughItemsOfType(const TArray<FBaseItem>& ItemsToCheck);

    /*------------------------ DELEGATES
     * -----------------------------------------*/

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnEquipmentChanged OnEquipmentChanged;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemAdded OnItemAdded;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemRemoved OnItemRemoved;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnEquippedArmorChanged OnEquippedArmorChanged;

    /*------------------------ MOVESETS
     * -----------------------------------------*/
    UFUNCTION(BlueprintCallable, Category = "ACF | Movesets")
    FGameplayTag GetCurrentDesiredMovesetTag() const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Movesets")
    FGameplayTag GetCurrentDesiredMovesetActionTag() const;

    UFUNCTION(BlueprintCallable, Category = "ACF | Movesets")
    FGameplayTag GetCurrentDesiredOverlayTag() const;

    // addition code
    virtual void BeginPlay() override;

    void GatherCharacterOwner();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void DestroyEquipment();

    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    TArray<FGameplayTag> GetAvailableEquipmentSlot() const { return AvailableEquipmentSlot; }

    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetAvailableEquipmentSlot(const TArray<FGameplayTag>& val) { AvailableEquipmentSlot = val; }

    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    TArray<FGameplayTag> GetAllowedWeaponTypes() const { return AllowedWeaponTypes; }

    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetAllowedWeaponTypes(const TArray<FGameplayTag>& val) { AllowedWeaponTypes = val; }

protected:
    /* Slots available to the character*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    TArray<FGameplayTag> AvailableEquipmentSlot;

    /* Weapon types usable by the character*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    TArray<FGameplayTag> AllowedWeaponTypes;

    /*Defines if the Entity should destroy all his equipped items when dying*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF|Drop")
    bool bDestroyItemsOnDeath = true;

    /*Defines if the Entity should drop all his droppable invnentory Items when it
     * dies*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF|Drop")
    bool bDropItemsOnDeath = true;

    /*If true, all the drops will be collapsed in a single world item*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF|Drop")
    bool bCollapseDropInASingleWorldItem = true;

    /*Defines if one of the equipped armors can hide / unhide the main mesh of the
     * owner entity*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    bool bUpdateMainMeshVisibility = true;

    /* the character's mesh pointer*/
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    USkeletalMeshComponent* MainCharacterMesh;

    /*Maximum number of Slot items in Inventory*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Savegame, Category = ACF)
    int32 MaxInventorySlots = 40;

    /*Define if an item should be automatically Equipped On Body, if it is picked
     * up from world */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    bool bAutoEquipItem = true;

    /*Max cumulative weight on*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Savegame, Category = ACF)
    float MaxInventoryWeight = 180.f;

    /* The character's starting items*/
    UPROPERTY(EditAnywhere, meta = (TitleProperty = "ItemClass"), BlueprintReadWrite, Category = ACF)
    TArray<FStartingItem> StartingItems;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnComponentLoaded();

    virtual void BeginDestroy() override;

private:
    /*Inventory of this character*/
    UPROPERTY(SaveGame, Replicated, ReplicatedUsing = OnRep_Inventory)
    TArray<FInventoryItem> Inventory;

    UPROPERTY(Replicated, ReplicatedUsing = OnRep_Equipment)
    FEquipment Equipment;

    TArray<FModularPart> ModularMeshes;

    UFUNCTION()
    void OnRep_Equipment();

    UFUNCTION()
    void OnRep_Inventory();

    void FillModularMeshes();

    FInventoryItem* Internal_GetInventoryItem(const FInventoryItem& item);

    FInventoryItem* Internal_GetInventoryItemByGuid(const FGuid& itemToSearch);

    FInventoryItem* Internal_GetInventoryItemByInventoryIndex(const int32 inventoryIndex);

    void Internal_DestroyEquipment();

    TArray<FInventoryItem*> FindItemsByClass(const TSubclassOf<AACFItem>& itemToFind);

    UPROPERTY()
    class ACharacter* CharacterOwner;

    /** The skeletal mesh used by the Owner. */
    UPROPERTY()
    class USkeletalMesh* originalMesh;

    UPROPERTY(Replicated)
    FGameplayTag CurrentlyEquippedSlotType;

    UPROPERTY(Replicated)
    float currentInventoryWeight = 0.f;

    /* Unequips the item if it is a Weapon, Armor or Consumable and it is
     * currently equipped*/
    void RemoveItemFromEquipment(const FEquippedItem& item);

    void MarkItemOnInventoryAsEquipped(const FInventoryItem& item, bool bIsEquipped, const FGameplayTag& itemSlot);

    int32 Internal_AddItem(const FBaseItem& item, bool bTryToEquip = false, float dropChancePercentage = 0.f, FGameplayTag equipSlot = FGameplayTag(), int32 inventoryIndex = -1);

    void AttachWeaponOnBody(AACFWeapon* WeaponToEquip);

    void AttachWeaponOnHand(AACFWeapon* _localWeapon);

    UFUNCTION(NetMulticast, Reliable, Category = ACF)
    void AddSkeletalMeshComponent(TSubclassOf<class AACFArmor> ArmorClass, FGameplayTag itemSlot);

    UFUNCTION(NetMulticast, Reliable, Category = ACF)
    void Internal_OnArmorUnequipped(const FGameplayTag& slot);

    void SpawnWorldItem(const TArray<FBaseItem>& items);

    void UseEquippedConsumable(FEquippedItem& EquipSlot, ACharacter* target);

    void Internal_UseItem(AACFConsumable* consumable, ACharacter* target, const FInventoryItem& Inventoryitem);

    // my addition code
    /*Move a item from one EquipmentComponent to another,usually used for a
     * storage*/
    //   UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable, Category =
    //   ACF)
    //  void MoveItemToAnotherInventory(UACFEquipmentComponent*
    //  OtherEquipmentComponent, class AACFItem* itemToMove, int32 count = 1);
};
