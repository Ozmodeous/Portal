// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Items/ACFItem.h"
#include "Net/UnrealNetwork.h"
#include <AbilitySystemComponent.h>
#include <ActiveGameplayEffectHandle.h>
#include <GameplayEffect.h>
#include <GameplayEffectTypes.h>
#include "GameFramework/Pawn.h"


// Sets default values
AACFItem::AACFItem()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    ItemInfo.ItemType = EItemType::Other;
}

void AACFItem::OnRep_ItemOwner()
{
}

FActiveGameplayEffectHandle AACFItem::AddGASModifierToOwner(const TSubclassOf<UGameplayEffect>& gameplayModifier)
{
    if (gameplayModifier) {
        UAbilitySystemComponent* AbilitySystemComponent = GetAbilityComponent();
        if (AbilitySystemComponent) {
            FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
            EffectContext.AddSourceObject(this);

            FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(gameplayModifier, 1.0f, EffectContext);
            if (SpecHandle.IsValid()) {
                return AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            }
        }
    }
    return FActiveGameplayEffectHandle();
}

void AACFItem::RemoveGASModifierToOwner(const FActiveGameplayEffectHandle& modifierHandle)
{
    if (modifierHandle.IsValid()) {
        UAbilitySystemComponent* AbilitySystemComponent = GetAbilityComponent();

        if (AbilitySystemComponent) {
            AbilitySystemComponent->RemoveActiveGameplayEffect(modifierHandle);
        }
    }
}

UAbilitySystemComponent* AACFItem::GetAbilityComponent() const
{
    if (ItemOwner) {
        return ItemOwner->FindComponentByClass<UAbilitySystemComponent>();
    }
    return nullptr;
}

void AACFItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AACFItem, ItemOwner);
}
