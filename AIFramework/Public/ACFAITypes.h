// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFActionCondition.h"
#include "ACFCCTypes.h"
#include "Actors/ACFCharacter.h"
#include "CoreMinimal.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>

#include "ACFAITypes.generated.h"

class AACFPatrolPath;
class AAIController;
class AController;

/**
 *
 */

UENUM(BlueprintType)
enum class EAgentRotation : uint8 {
    EGroupForward = 0 UMETA(DisplayName = "Group Forward"),
    EGroupCenter = 1 UMETA(DisplayName = "Look at Group Center")
};

UENUM(BlueprintType)
enum class EPatrolType : uint8 {
    EFollowSpline = 0 UMETA(DisplayName = "Follow Spline"),
    ERandomPoint = 1 UMETA(DisplayName = "Random Point In Range"),
};

UENUM(BlueprintType)
enum class EAICombatState : uint8 {
    EMeleeCombat = 0 UMETA(DisplayName = "Melee Attacks"),
    EChaseTarget = 1 UMETA(DisplayName = "Chase Target"),
    ERangedCombat = 2 UMETA(DisplayName = "Ranged Attacks"),
    EStudyTarget = 3 UMETA(DisplayName = "Studytarget"),
    EFlee = 4 UMETA(DisplayName = "Flee Away"),
};

USTRUCT(BlueprintType)
struct FAIAgentsInfo {
    GENERATED_BODY()

public:
    FAIAgentsInfo()
    {
        AICharacter = nullptr;
        // characterClass = AACFCharacter::StaticClass();
    };

    //   FAIAgentsInfo(class AACFCharacter* inChar, class AACFAIController* inContr);

    FAIAgentsInfo(class AACFCharacter* inCharr);

    FORCEINLINE bool operator==(const FAIAgentsInfo& Other) const
    {
        return this->AICharacter == Other.AICharacter;
    }

    FORCEINLINE bool operator!=(const FAIAgentsInfo& Other) const
    {
        return this->AICharacter != Other.AICharacter;
    }

    FORCEINLINE bool operator==(const AACFCharacter* Other) const
    {
        return this->AICharacter == Other;
    }

    FORCEINLINE bool operator!=(const AACFCharacter* Other) const
    {
        return this->AICharacter != Other;
    }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACF)
    TObjectPtr<class AACFCharacter> AICharacter;

    UPROPERTY(SaveGame)
    TSubclassOf<AACFCharacter> characterClass;

    UPROPERTY(SaveGame)
    FName Guid;

    TObjectPtr<class AACFAIController> GetController() const;
};

USTRUCT(BlueprintType)
struct FCommandChances : public FTableRowBase {
    GENERATED_BODY()

public:
    FCommandChances()
    {
        Condition = nullptr;
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    FGameplayTag CommandTag;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = ACF)
    class UACFActionCondition* Condition;
};

USTRUCT(BlueprintType)
struct FAICombatStateConfig {
    GENERATED_BODY()

public:
    FAICombatStateConfig()
        : Conditions()
    {

        CombatState = EAICombatState::EMeleeCombat;
    };

    FAICombatStateConfig(const EAICombatState inCombatState, float inChance, const ELocomotionState& inState)
        : Conditions()
    {
        TriggerChancePercentage = inChance;
        LocomotionState = inState;

        CombatState = inCombatState;
    }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    EAICombatState CombatState;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = ACF)
    TArray<class UACFActionCondition*> Conditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    float TriggerChancePercentage = 75.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    ELocomotionState LocomotionState = ELocomotionState::EWalk;

    UACFDistanceActionCondition* GetDistanceBasedCondition() const
    {
        for (const auto condition : Conditions) {
            UACFDistanceActionCondition* distanceCond = Cast<UACFDistanceActionCondition>(condition);
            if (distanceCond) {
                return distanceCond;
            }
        }
        return nullptr;
    }

    FORCEINLINE bool operator==(const EAICombatState& Other) const
    {
        return this->CombatState == Other;
    }

    FORCEINLINE bool operator!=(const EAICombatState& Other) const
    {
        return this->CombatState != Other;
    }

    FORCEINLINE bool operator==(const FAICombatStateConfig& Other) const
    {
        return this->CombatState == Other.CombatState;
    }

    FORCEINLINE bool operator!=(const FAICombatStateConfig& Other) const
    {
        return this->CombatState != Other.CombatState;
    }
};

USTRUCT(BlueprintType)
struct FConditions : public FActionChances {
    GENERATED_BODY()

public:
    FConditions()
    {
        Condition = nullptr;
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = ACF)
    class UACFActionCondition* Condition;
};

USTRUCT(BlueprintType)
struct FBaseUnit {
    GENERATED_BODY()

public:
    FBaseUnit() { };

    FBaseUnit(const TSubclassOf<AACFCharacter>& inClass)
    {
        AIClassBP = inClass;
    }
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = ACF)
    TSubclassOf<AACFCharacter> AIClassBP;

    FORCEINLINE bool operator==(const FBaseUnit& Other) const
    {
        return this->AIClassBP == Other.AIClassBP;
    }

    FORCEINLINE bool operator!=(const FBaseUnit& Other) const
    {
        return this->AIClassBP != Other.AIClassBP;
    }

    FORCEINLINE bool operator==(const TSubclassOf<AACFCharacter>& Other) const
    {
        return this->AIClassBP == Other;
    }

    FORCEINLINE bool operator!=(const TSubclassOf<AACFCharacter>& Other) const
    {
        return this->AIClassBP != Other;
    }
};

USTRUCT(BlueprintType)
struct FAISpawnInfo : public FBaseUnit {
    GENERATED_BODY()

    FAISpawnInfo()
    {
        AIClassBP = AACFCharacter::StaticClass();
        SpawnTransform = FTransform();
    };

    FAISpawnInfo(const TSubclassOf<class AACFCharacter> classToSpawn)
    {
        AIClassBP = classToSpawn;
        SpawnTransform = FTransform();
    }

    FAISpawnInfo(const TSubclassOf<class AACFCharacter> classToSpawn, const FTransform& relativePos)
    {
        AIClassBP = classToSpawn;
        SpawnTransform = relativePos;
    }

    UPROPERTY(EditAnywhere, meta = (MakeEditWidget), BlueprintReadWrite, Category = ACF)
    FTransform SpawnTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    TObjectPtr<AACFPatrolPath> PatrolPath;

    FORCEINLINE bool operator==(const FAISpawnInfo& Other) const
    {
        return this->AIClassBP == Other.AIClassBP;
    }

    FORCEINLINE bool operator!=(const FAISpawnInfo& Other) const
    {
        return this->AIClassBP != Other.AIClassBP;
    }
};

USTRUCT(BlueprintType)
struct FWaveConfig {
    GENERATED_BODY()

public:
    FWaveConfig()
    {
        GroupSpawner = nullptr;
        secondsDelayToNextWave = 1.f;
    }
    /*The spawner to be used for this wave */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    TSoftObjectPtr<class AACFAIGroupSpawner> GroupSpawner;

    /*The Agents To Be Used For This Wave. Overrides default spawner configuration*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    TArray<FAISpawnInfo> WaveAgentsOverride;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    float secondsDelayToNextWave;

    void AddAIToSpawn(const TSubclassOf<AACFCharacter>& charClass, int32 totalNumber = 1)
    {
        for (int32 index = 0; index < totalNumber; index++) {
            WaveAgentsOverride.Add(FAISpawnInfo(charClass));
        }
    }

    void RemoveAIToSpawn(const TSubclassOf<AACFCharacter>& charClass, int32 totalNumber = 1)
    {
        for (int32 index = 0; index < totalNumber; index++) {
            if (WaveAgentsOverride.Contains(charClass)) {
                WaveAgentsOverride.Remove(charClass);
            }
        }
    }
};

USTRUCT(BlueprintType)
struct FACFAITicket {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    AActor* Target;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    AController* AIController;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    float TimeRemaining;

    FACFAITicket()
        : Target(nullptr)
        , AIController(nullptr)
        , TimeRemaining(0.f)
    {
    }
    FACFAITicket(AActor* InTarget, AController* InController, float InTime)
        : Target(InTarget)
        , AIController(InController)
        , TimeRemaining(InTime)
    {
    }

    FORCEINLINE bool operator==(const AController* OtherController) const
    {
        return AIController == OtherController;
    }

    FORCEINLINE bool operator!=(const AController* OtherController) const
    {
        return AIController != OtherController;
    }
};

UCLASS()
class AIFRAMEWORK_API UACFAITypes : public UObject {
    GENERATED_BODY()
};
