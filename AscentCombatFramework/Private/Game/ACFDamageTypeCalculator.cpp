// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Game/ACFDamageTypeCalculator.h"
#include "ARSStatisticsComponent.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFDefenseStanceComponent.h"
#include "Game/ACFDamageType.h"
#include "Game/ACFFunctionLibrary.h"
#include "Game/ACFTypes.h"
#include "Logging.h"

UACFDamageTypeCalculator::UACFDamageTypeCalculator()
{
}

bool UACFDamageTypeCalculator::IsCriticalDamage_Implementation(const FACFDamageEvent& inDamageEvent)
{
    if (inDamageEvent.DamageDealer) {
        const FDamageInfluence* critChance = CritChancePercentageByParameter.Find(inDamageEvent.DamageClass);
        const UARSStatisticsComponent* dealerComp = inDamageEvent.DamageDealer->FindComponentByClass<UARSStatisticsComponent>();

        if (critChance && dealerComp) {
            const float percentage = dealerComp->GetCurrentAttributeValue(critChance->Parameter) * critChance->ScalingFactor;
            if (FMath::RandRange(0.f, 100.f) < percentage) {
                return true;
            }
        }
    }
    return false;
}

float UACFDamageTypeCalculator::CalculateFinalDamage_Implementation(const FACFDamageEvent& inDamageEvent)
{

    // Verify received input data
    if (!inDamageEvent.DamageReceiver) {
        UE_LOG(ACFLog, Error, TEXT("Missing Damage receiver!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }

    if (!inDamageEvent.DamageDealer) {
        UE_LOG(ACFLog, Error, TEXT("Missing Damage Dealer!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }

    if (!inDamageEvent.DamageClass) {
        UE_LOG(ACFLog, Error, TEXT("Missing Damage Class!!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }
    UACFDamageType* damageClass = GetDamageType(inDamageEvent);

    if (!damageClass) {
        UE_LOG(ACFLog, Error, TEXT("DamageClass Influence NOT Set!!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }
    FDamageInfluences damagesInf = damageClass->DamageScaling;
    //  const FDamageInfluences* damagesInf = DamageInfluencesByParameter.Find(inDamageEvent.DamageClass);

    // starting from the base damage
    float totalDamage = inDamageEvent.FinalDamage;

    const UARSStatisticsComponent* dealerComp = inDamageEvent.DamageDealer->FindComponentByClass<UARSStatisticsComponent>();
    UARSStatisticsComponent* receiverComp = inDamageEvent.DamageReceiver->FindComponentByClass<UARSStatisticsComponent>();

    // First we calculate the sum of every parameter influence
    for (const auto& damInf : damagesInf.AttackParametersInfluence) {
        totalDamage += dealerComp->GetCurrentAttributeValue(damInf.Parameter) * damInf.ScalingFactor;
    }

    // Then reduces it for defenses
    //  const FDamageInfluences* defenseInf = DamageInfluencesByParameter.Find(inDamageEvent.DamageClass);

    for (const auto& damInf : damagesInf.DefenseParametersPercentages) {
        totalDamage = UACFFunctionLibrary::ReduceDamageByPercentage(totalDamage,
            receiverComp->GetCurrentAttributeValue(damInf.Parameter) * damInf.ScalingFactor);
    }

    if (inDamageEvent.bIsCritical) {
        totalDamage *= critMultiplier;
    }

    // Final Randomization
    if (totalDamage != 0.f) {
        const float deviation = totalDamage * DefaultRandomDamageDeviationPercentage / 100;
        totalDamage = FMath::FRandRange(totalDamage - deviation, totalDamage + deviation);
    }

    UACFDefenseStanceComponent* defComp = inDamageEvent.DamageReceiver->FindComponentByClass<UACFDefenseStanceComponent>();
    FGameplayTag outResponse;

    if (defComp && defComp->IsInDefensePosition() && defComp->TryBlockIncomingDamage(inDamageEvent, totalDamage, outResponse)) {
        if (defComp->IsParrying()) {
            totalDamage = 0.f;
        } else {
            const float reducedPercentage = receiverComp->GetCurrentAttributeValue(DefenseStanceParameterWhenBlocked);
            totalDamage = UACFFunctionLibrary::ReduceDamageByPercentage(totalDamage, reducedPercentage);
        }

    } else {
        // Damage Zones
        const float* zoneMult = DamageZoneToDamageMultiplier.Find(inDamageEvent.DamageZone);
        if (zoneMult) {
            totalDamage *= *zoneMult;
        }

        // Hit Responses
        const float* hitMult = HitResponseActionMultiplier.Find(inDamageEvent.HitResponseAction);
        if (hitMult) {
            totalDamage *= *hitMult;
        }
    }

    return totalDamage;
}

FGameplayTag UACFDamageTypeCalculator::EvaluateHitResponseAction_Implementation(const FACFDamageEvent& damageEvent, const TArray<FOnHitActionChances>& hitResponseActions)
{
    UACFDefenseStanceComponent* defComp = damageEvent.DamageReceiver->FindComponentByClass<UACFDefenseStanceComponent>();
    FGameplayTag outResponse;

    if (!damageEvent.DamageDealer) {
        return FGameplayTag();
    }

    if (defComp && defComp->IsInDefensePosition() && defComp->CanBlockDamage(damageEvent)) {
        if (defComp->IsParrying()) {
            return defComp->GetActionToBeTriggeredOnParry();
        } else {
            return defComp->GetBlockAction();
        }
    }

    if (defComp && defComp->TryCounterAttack(damageEvent, outResponse)) {
        return outResponse;
    }
    const AACFCharacter* charOwn = Cast<AACFCharacter>(damageEvent.DamageReceiver);
    if (charOwn) {
        TArray<float> weights;
        TArray<FActionChances> executableActions;
        for (const auto& elem : hitResponseActions) {
            if (UACFFunctionLibrary::ShouldExecuteAction(elem, charOwn) && EvaluetHitResponseAction(elem, damageEvent)) {
                executableActions.Add(elem);
                weights.Add(elem.Weight);
            }
        }
        const int32 index = UACFFunctionLibrary::ExtractIndexWithProbability(weights);
        if (executableActions.IsValidIndex(index)) {
            outResponse = executableActions[index].ActionTag;
        }
    }
    UARSStatisticsComponent* receiverComp = damageEvent.DamageReceiver->FindComponentByClass<UARSStatisticsComponent>();
    UACFDamageType* damageType = GetDamageType(damageEvent);
    if (receiverComp && damageType && StaggerResistanceStastistic != FGameplayTag() && outResponse == UACFFunctionLibrary::GetDefaultHitState()) {
        const float finalDamgeTemp = CalculateFinalDamage(damageEvent) * damageType->StaggerMutliplier;
        receiverComp->ModifyStatistic(StaggerResistanceStastistic, -finalDamgeTemp);
        if (receiverComp->GetCurrentValueForStatitstic(StaggerResistanceStastistic) > 1.f) {
            return FGameplayTag();
        }
    }

    if (receiverComp && StaggerResistanceStastistic != FGameplayTag() && HeavyHitReaction != FGameplayTag()) {

        const float currentResistance = receiverComp->GetCurrentValueForStatitstic(StaggerResistanceStastistic);
        const float HeavyHitThreshold = -StaggerResistanceForHeavyHitMultiplier * receiverComp->GetMaxValueForStatitstic(StaggerResistanceStastistic);
        if (currentResistance < HeavyHitThreshold) {
            outResponse = HeavyHitReaction;
        }
    }

    return outResponse;
}

bool UACFDamageTypeCalculator::EvaluetHitResponseAction(const FOnHitActionChances& action, const FACFDamageEvent& damageEvent)
{
    if ((uint8)damageEvent.DamageDirection != (uint8)action.AllowedFromDirection && action.AllowedFromDirection != EActionDirection::EveryDirection)
        return false;

    for (const TSubclassOf<UDamageType>& damageType : action.AllowedDamageTypes) {
        if (damageEvent.DamageClass->IsChildOf(damageType))
            return true;
    }
    return false;
}
