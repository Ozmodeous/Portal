// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFStorageComponent.h"
#include "Components/ACFCurrencyComponent.h"
#include "Components/ACFEquipmentComponent.h"
#include "Items/ACFItem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include <GameFramework/Pawn.h>
#include "ACFItemSystemFunctionLibrary.h"

// Sets default values for this component's properties
UACFStorageComponent::UACFStorageComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);
    // ...
}

// Called when the game starts
void UACFStorageComponent::BeginPlay()
{
    Super::BeginPlay();
    CheckEmpty();
}

void UACFStorageComponent::OnComponentLoaded_Implementation()
{
}

void UACFStorageComponent::OnComponentSaved_Implementation()
{
}

void UACFStorageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UACFStorageComponent, Items);
}

void UACFStorageComponent::RemoveItems_Implementation(const TArray<FBaseItem>& inItems)
{
    TArray<FBaseItem> pendingRemove;
    for (auto& item : inItems) {
        FBaseItem* currentItem = Items.FindByKey(item);

        if (currentItem) {
            currentItem->Count -= item.Count;

            if (currentItem->Count <= 0) {
                pendingRemove.Add(*currentItem);
            }
        }
    }

    for (auto& removed : pendingRemove) {
        Items.Remove(removed);
    }

    OnItemChanged.Broadcast(Items);
    CheckEmpty();
}



void UACFStorageComponent::RemoveItem_Implementation(const FBaseItem& inItem)
{
    FBaseItem* currentItem = Items.FindByKey(inItem);
    if (currentItem) {
        currentItem->Count -= inItem.Count;
        if (currentItem->Count <= 0) {
            const int32 index = Items.IndexOfByKey(inItem);
            if (Items.IsValidIndex(index)) {
                Items.RemoveAt(index);

            }
        }
        OnItemChanged.Broadcast(Items);
        CheckEmpty();
    }
}


void UACFStorageComponent::AddItems_Implementation(const TArray<FBaseItem>& inItems)
{
    for (const auto& item : inItems) {
        AddItem(item);
    }
}

void UACFStorageComponent::AddItem_Implementation(const FBaseItem& inItem)
{
    FBaseItem* currentItem = Items.FindByKey(inItem);

    if (currentItem) {
        currentItem->Count += inItem.Count;
    } else {
        Items.Add(inItem);
    }
    OnItemChanged.Broadcast(Items);
}



void UACFStorageComponent::MoveItemsToInventory(const TArray<FBaseItem>& inItems, UACFEquipmentComponent* equipComp)
{
    if (equipComp) {
        equipComp->MoveItemsToInventory(inItems, this);
    }
}

// addition code
void UACFStorageComponent::OnRep_Items()
{
    OnItemChanged.Broadcast(Items);
    CheckEmpty();
}

void UACFStorageComponent::CheckEmpty()
{
    if (IsStorageEmpty()) {
        OnStorageEmpty.Broadcast();
    }
}

bool UACFStorageComponent::IsStorageEmpty()
{
    return GetItems().IsEmpty() && GetCurrentCurrencyAmount() <= 0;
}

void UACFStorageComponent::GatherCurrency(float amount, UACFCurrencyComponent* currencyComp)
{
    currencyComp->AddCurrency(amount);
    RemoveCurrency(amount);
}


bool UACFStorageComponent::CanPawnGatherItems(const APawn* pawn, const FBaseItem& itemsToGather)
{
    UACFEquipmentComponent* equipComp = GetPawnEquipment(pawn);
    if (equipComp) {
         return equipComp->NumberOfItemCanTake(itemsToGather.ItemClass ) >= itemsToGather.Count ;
    }
    return false;
}

void UACFStorageComponent::GatherAllCurrency(UACFCurrencyComponent* currencyComp)
{
    GatherCurrency(GetCurrentCurrencyAmount(), currencyComp);
}

UACFCurrencyComponent* UACFStorageComponent::GetPawnCurrencyComponent(const APawn* pawn) const
{
    return UACFItemSystemFunctionLibrary::GetPawnCurrencyComponent(pawn);
}


float UACFStorageComponent::GetPawnCurrency(const APawn* pawn) const
{
    return UACFItemSystemFunctionLibrary::GetPawnCurrency(pawn);
}

class UACFEquipmentComponent* UACFStorageComponent::GetPawnEquipment(const APawn* pawn) const
{
    return UACFItemSystemFunctionLibrary::GetPawnEquipment(pawn);
}

TArray<FInventoryItem> UACFStorageComponent::GetPawnInventory(const APawn* pawn) const
{
    const UACFEquipmentComponent* equipComp = GetPawnEquipment(pawn);
    if (equipComp) {
        return equipComp->GetInventory();
    }
    return TArray<FInventoryItem>();
}


void UACFStorageComponent::HandleCurrencyChanged()
{
    CheckEmpty();
}
