// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFDeveloperSettings.h"
#include "ACFTypes.h"
#include "CoreMinimal.h"
#include "Game/ACFDamageCalculation.h"
#include "GameFramework/DamageType.h"
#include "GameplayTagContainer.h"
#include "ACFDamageType.h"

#include "ACFDamageTypeCalculator.generated.h"

class UDamageType;
struct FDamageInfluence;
struct FOnHitActionChances;

/**
 *
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFDamageTypeCalculator : public UACFDamageCalculation {
    GENERATED_BODY()

    UACFDamageTypeCalculator();

protected:
    /*For every hitresponse you can define a multiplier to be applied to the final damage*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses")
    TMap<FGameplayTag, float> HitResponseActionMultiplier;

    /*For every damagetype, the parameter to be used to calculate crit chance. 100 means always crit*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF| Critical Damage Config")
    TMap<TSubclassOf<UDamageType>, FDamageInfluence> CritChancePercentageByParameter;

    /*Multiplier applied when an hit is Critical*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF| Critical Damage Config")
    float critMultiplier = 1.5f;

    /*This statistic is used to prevent the owner to go on Hit Action ( = being staggered).
    If set, this statistic will be reduced by the actual damage at every hit and the owner
    will only be staggered when this statistic reaches 0. (make sure regen is activate, otherwise
    the AI after the first stagger, will be always be staggered) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses")
    FGameplayTag StaggerResistanceStastistic;

    /*Actions to be triggered when the stagger resistance of the character is heavily damaged*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses|Heavy Hit")
    FGameplayTag HeavyHitReaction;

    /*Heavy Hit Action will be triggered when StaggerResistanceStastistic will be below this value.
    To be intended as a negative scaling factor for your Stagger Resistance statistic: the higher, 
    the more difficult to trigger an Heavy Hit Reaction*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses|Heavy Hit")
    float StaggerResistanceForHeavyHitMultiplier = 2.f;


    /*DEPRECATED, USE THE DAMAGE INFLUENCES WITHIN ANY DAMAGETYPE CLASS!!!!*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    TMap<TSubclassOf<UDamageType>, FDamageInfluences> DamageInfluencesByParameter;
       
    /*Random deviation in percentage added to final damage. % means that final damage will be randomized
    in the range -5% , +5%*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    float DefaultRandomDamageDeviationPercentage = 5.0f;

    /*Multiplier applied depending on the damaged zone of the receiver*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    TMap<EDamageZone, float> DamageZoneToDamageMultiplier;

    /*The parameter to be used to reduce incoming damage when in Defense State*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    FGameplayTag DefenseStanceParameterWhenBlocked;

    virtual float CalculateFinalDamage_Implementation(const FACFDamageEvent& inDamageEvent) override;

    virtual FGameplayTag EvaluateHitResponseAction_Implementation(const FACFDamageEvent& inDamageEvent, 
        const TArray<FOnHitActionChances>& hitResponseActions) override;

    virtual bool IsCriticalDamage_Implementation(const FACFDamageEvent& inDamageEvent) override;

private:
  
    bool EvaluetHitResponseAction(const FOnHitActionChances& action, const FACFDamageEvent& damageEvent);
};
