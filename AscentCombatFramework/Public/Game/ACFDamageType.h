// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFCCTypes.h"
#include "ACFTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include <Engine/EngineTypes.h>
#include <GameplayTagContainer.h>

#include "ACFDamageType.generated.h"


/**
 *
 */

UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFDamageType : public UDamageType {
    GENERATED_BODY()

public:
    UACFDamageType() {
        StaggerMutliplier = 1.f;
    }

    /*How much this damage influences */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    float StaggerMutliplier;
 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    FGameplayTagContainer DamageTags;

    /*Those be used to calculate the damage and a scaling factor
    Total Damage will be the sum of all ATTACK DAMAGE INFLUENCES of the DAMAGE DEALER  * scalingfactor */
    /*DEFENSE PARAMETER PERCENTAGES are the parameters that will be used from the DAMAGE RECEIVER to
    REDUCE the incoming damage and a scaling factor.
    Total Damage will be reduced by the sum of all the influences * scalingfactor,  in percentages.
    if the sum of all those parameters scaled is 30, incoming damage will be reduced by 30%.
    If this number is >= 100, damage will be reduced to 0*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    FDamageInfluences DamageScaling;
};

UCLASS()
class ASCENTCOMBATFRAMEWORK_API UMeleeDamageType : public UACFDamageType {
    GENERATED_BODY()
};

UCLASS()
class ASCENTCOMBATFRAMEWORK_API URangedDamageType : public UACFDamageType {
    GENERATED_BODY()
};

UCLASS()
class ASCENTCOMBATFRAMEWORK_API UAreaDamageType : public UACFDamageType {
    GENERATED_BODY()
};

UCLASS()
class ASCENTCOMBATFRAMEWORK_API USpellDamageType : public UACFDamageType {
    GENERATED_BODY()
};

UCLASS()
class ASCENTCOMBATFRAMEWORK_API UFallDamageType : public UACFDamageType {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FACFDamageEvent {
    GENERATED_BODY()

public:
    FACFDamageEvent()
    {
        DamageDealer = nullptr;
        DamageReceiver = nullptr;
        PhysMaterial = nullptr;
        DamageZone = EDamageZone::ENormal;
        FinalDamage = 0.f;
        hitDirection = FVector(0.f);
        DamageDirection = EACFDirection::Front;
    }

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FGameplayTag HitResponseAction;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FName contextString;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    class AActor* DamageDealer;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    class AActor* DamageReceiver;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    class UPhysicalMaterial* PhysMaterial;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    EDamageZone DamageZone;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    float FinalDamage;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FHitResult hitResult;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FVector hitDirection;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    TSubclassOf<class UACFDamageType> DamageClass;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    EACFDirection DamageDirection;

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    bool bIsCritical = false;
};
