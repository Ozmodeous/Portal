// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#include "Items/ACFConsumable.h"
#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameState.h"
#include "Kismet/GameplayStatics.h"
#include "ACMEffectsDispatcherComponent.h"
#include "Kismet/GameplayStatics.h"


AACFConsumable::AACFConsumable()
{
    ItemInfo.ItemType = EItemType::Consumable;
    ItemInfo.Name = FText::FromString("Base Consumable");
}


bool AACFConsumable::CanBeUsed_Implementation(const APawn* pawn)
{
    return true;
}

void AACFConsumable::Internal_UseItem(ACharacter* target)
{
    if (target) {

        UARSStatisticsComponent* statComp = target->FindComponentByClass<UARSStatisticsComponent>();
        if (statComp) {
            for (const auto& modifier : TimedAttributeSetModifier) {
                statComp->AddTimedAttributeSetModifier(modifier.modifier, modifier.duration);
            }
            for (const auto& statisMod : StatModifier) {
                statComp->ModifyStat(statisMod);
            }
        }

        AddGASModifierToOwner(ConsumableGameplayEffect);
		const AGameStateBase* gameState = UGameplayStatics::GetGameState(this);
		if (gameState) {
			UACMEffectsDispatcherComponent* EffectComp = gameState->FindComponentByClass<UACMEffectsDispatcherComponent>();
            if (EffectComp) {
				EffectComp->PlayReplicatedActionEffect(OnUsedEffect, target);
            }
            else {
                UE_LOG(LogTemp, Error, TEXT("Missing Effect Dispatcher Component in Game State! - ACFConsumbale"));
            }
		}

        OnItemUsed();
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Item Owner - ACFConsumbale"));
    }
}

void AACFConsumable::OnItemUsed_Implementation()
{
    //IMPLEMENT ME!
}
