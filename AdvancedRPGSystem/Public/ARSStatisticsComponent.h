// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ARSTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>
#include "ARSLevelingComponent.h"

#include "ARSStatisticsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatValueChanged, FGameplayTag, Stat, const FStatistic&, value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttributeSetModified);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatisticReachesZero, FGameplayTag, Stat);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatisticValueChanged, const FGameplayTag&, Stat, float, OldValue, float, NewValue);

UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class ADVANCEDRPGSYSTEM_API UARSStatisticsComponent : public UARSLevelingComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UARSStatisticsComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    virtual void OnLevelChanged() override;

    /*If this is set to true, InitializeAttributeSet is called automatically On BeginPlay serverside.
        If false you have to manually initialize this component when needed*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ARS | AttributeSet")
    bool bAutoInitialize = false;

        /*Indicates if this character can gain exp and level up and how the stats increase is handles:
    EGenerateNewStatsFromCurves  After leveling up, new stats are generated from AttributesByLevelCurves
    EAssignPerksManually	*/
    UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadWrite, Category = "ARS | Leveling System")
    ELevelingType LevelingType = ELevelingType::ECantLevelUp;

    /*Define how your Statistics and Attributes are generated:
        Default Without Generation: No generation is applied, Default value are used
    Generate From Default Attributes: Define your Attributes in DefaultAttributeSet, Attributes and Statistic will be generated following the rules
        defined in ProjectSettings - ARS Settings
        Load By Level From Curve: Define the level of the character and Generate stats from AttributesByLevelCurves */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS | AttributeSet")
    EStatsLoadMethod StatsLoadMethod = EStatsLoadMethod::EGenerateFromDefaultsPrimary;

    /*Attribute set used if you select No Generation or Generate From Default Values
        In the first case it is used as is, in the second one, your Attribute will be
        used to generate your Statistics and Attributes following your ARS Settings.*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditCondition = "StatsLoadMethod != EStatsLoadMethod::ELoadByLevel"), Category = "ARS | Default AttributeSet")
    FAttributesSet DefaultAttributeSet;

    /*Implement Rules to generate your ATTRIBUTES starting from your Level.
        For each Curve on X you have the Level, on Y the actual value of the Attribute for that Level
        Is used when StatLoadMethod is set to LoadByLevel and your LevelingType is GenerateNewStatsFromCurves*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "LevelingType == ELevelingType::EGenerateNewStatsFromCurves && StatsLoadMethod == EStatsLoadMethod::ELoadByLevel"),
        Category = "ARS | Leveling System")
    class UARSLevelingSystemDataAsset* AttributesByLevelConfig;

    /*Multiplier applied everytime you modify CurrentValue of the target Statistics.
        Could be useful to implement logics like: if your inventory is full, Stamina consumption is
        multiplied by 1.5 for every action*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS | AttributeSet")
    TMap<FGameplayTag, float> StatisticConsumptionMultiplier;

    /*Turn off regeneration for this Character for optimization*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS | StatRegen")
    bool bCanRegenerateStatistics = true;

    /*Regeneration time interval, set high values for optimization*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS | StatRegen")
    float RegenerationTimeInterval = 0.2f;

private:
    UFUNCTION()
    TArray<FAttribute> GetPrimitiveAttributesForCurrentLevel();

    TMap<FGameplayTag, FDateTime> regenDelay;

    UPROPERTY()
    bool bIsInitialized = false;

    TArray<FAttributesSetModifier> storedUnactiveModifiers;

    UPROPERTY()
    FTimerHandle RegenTimer;

    UPROPERTY()
    bool bIsRegenerationStarted = false;

    TArray<FAttribute> Internal_GetPrimitiveAttributesForCurrentLevel();

    void RegenerateStat();

    UPROPERTY(SaveGame, ReplicatedUsing = OnRep_AttributeSet)
    FAttributesSet AttributeSet;



    UFUNCTION()
    void OnRep_AttributeSet();

    void Internal_InitializeStats();

    UPROPERTY(SaveGame, Replicated)
    FAttributesSet baseAttributeSet;

    UPROPERTY()
    TArray<FAttributesSetModifier> activeModifiers;


    UFUNCTION()
    void Internal_ModifyStat(const FStatisticValue& StatMod, bool bResetDelay = true);

    UFUNCTION()
    void Internal_AddModifier(const FAttributesSetModifier& modifier);

    void CalcualtePrimaryStats();
    void CalcualteSecondaryStats();

    FAttributesSetModifier CreateAdditiveAttributeSetModifireFromPercentage(const FAttributesSetModifier& _modifier);

    void GenerateSecondaryStat();
    void GenerateSecondaryStatFromCurrentPrimaryStat();

    // Regenerate Stats
    UFUNCTION(BlueprintCallable, Category = ARS)
    void GenerateStats();



public:
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /*Starts to regenerate all the Statistics with a regeneration value != 0.f.
        Server Side*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void StartRegeneration();

    /* this function should be called ONLY ON SERVER,*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    void InitializeAttributeSet();

    /*Stops to regenerate all the Statistics with a regeneration value != 0.f.
        Server Side*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void StopRegeneration();

    UPROPERTY(BlueprintAssignable, Category = ARS)
    FOnAttributeSetModified OnAttributeSetModified;


    UPROPERTY(BlueprintAssignable, Category = ARS)
    FOnStatisticReachesZero OnStatisiticReachesZero;

    UPROPERTY(BlueprintAssignable, Category = ARS)
    FOnStatisticValueChanged OnStatisticChanged;

    
    
    /*Returns current level of the character*/
    UFUNCTION(BlueprintPure, Category = ARS)
    FORCEINLINE bool CanLevelUp() const { return LevelingType != ELevelingType::ECantLevelUp; };

    /*Adds a Modifier for the current AttributeSet of the character*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void AddAttributeSetModifier(const FAttributesSetModifier& _modifier);

    /*Adds a Modifier for the current AttributeSet of the character*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void RemoveAttributeSetModifier(const FAttributesSetModifier& _modifier);

    /*Everytime the StatisticTag Statistic will be modified, the amount of
        modifier will be multiplied by this value*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void AddStatisticConsumptionMultiplier(FGameplayTag statisticTag, float multiplier = 1.0f);

    UFUNCTION(BlueprintCallable, Category = ARS)
    float GetConsumptionMultiplierByStatistic(FGameplayTag statisticTag) const;

    /*Checks if current Statistics are enough for apply those Costs*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    bool CheckCosts(const TArray<FStatisticValue>& Costs) const;

    /*Checks if your current Attributes are higher of the one passed as an argument,
        returns true only if all the actual attributes of the character are higher then the one passed as
        parameter*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    bool CheckPrimaryAttributesRequirements(const TArray<FAttribute>& attributeRequirements) const;

    /*Checks if current Statistic is enough to apply this Cost*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    bool CheckCost(const FStatisticValue& Cost) const;

    /*Remove the Statistic modifier from the actual Statistic*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    void ConsumeStatistics(const TArray<FStatisticValue>& Costs);

    /*Modifies the actual Statistic "stat" adding to it "value" to the CurrentValue
        Server Side*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    void ModifyStatistic(FGameplayTag Stat, float value);

    /*Modifies the actual Statistic "stat" setting his CurrentValue to  "value" */
    UFUNCTION(BlueprintCallable, Category = ARS)
    void RefillStat(FGameplayTag Stat);

    // called to add to subtract given value from statistics based on param stat
    UFUNCTION(Server, Reliable, Category = ARS)
    void ModifyStat(const FStatisticValue& StatMod);



    /*Getter Current value for Statistic*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    float GetCurrentValueForStatitstic(FGameplayTag stat) const;

    /*Getter Max value for Statistic*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    float GetMaxValueForStatitstic(FGameplayTag stat) const;

    /*Getter Current value/ Max Value for Statistic*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    float GetNormalizedValueForStatitstic(FGameplayTag statTag) const;

    /*Getter full statistic Structure*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    FStatistic GetFullStatisticStructure(FGameplayTag stat) const;

    /*Getter for current Primary Attribute Value*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    float GetCurrentPrimaryAttributeValue(FGameplayTag attributeTag) const;

    /*Getter for current Attribute Value*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    float GetCurrentAttributeValue(FGameplayTag attributeTag) const;

    /*Getter for the entire AttributeSet */
    UFUNCTION(BlueprintPure, Category = ARS)
    FAttributesSet GetCurrentAttributeSet() const;

    /*Getter for the entire Base AttributeSet without Modifiers.
    WARNING: current values for statistics are NOT updated here*/
    UFUNCTION(BlueprintPure, Category = ARS)
    FAttributesSet GetBaseAttributeSetWithoutModifiers() const
    {
        return baseAttributeSet;
    };

    /*Adds a Modifier for the current AttributeSet of the character for a limited amount of time*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void AddTimedAttributeSetModifier(const FAttributesSetModifier& _modifier, float duration);

    /*use this to Load an attribute set from savegame and properly reinitialize your attributeset*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void ReinitializeAttributeSetFromNewDefault(const FAttributesSet& newDefault);

    /*use this to  properly reinitialize your attributeset from a saved level*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void SetNewLevelAndReinitialize(int32 newLevel);

    /*use this to  properly reinitialize your attributeset from a saved level*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void PermanentlyModifyPrimaryAttribute(FGameplayTag attribute, float deltaValue = 1.0f);

 

    /*Uses numPerks of perks to increment the Attribute attributeTag by numPerks amount.
        Does nothing you don't have enough perks*/
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ARS)
    void AssignPerkToPrimaryAttribute(FGameplayTag attributeTag, int32 numPerks = 1);


    /*Indicates if there is a statistic with this tag in the AttributeSet*/
    UFUNCTION(BlueprintCallable, Category = ARS)
    bool HasValidStatistic(FGameplayTag stat) const { return AttributeSet.Statistics.Contains(stat); };

    UFUNCTION(BlueprintCallable, Category = ARS)
    bool HasValidAttribute(FGameplayTag param) const { return AttributeSet.Parameters.Contains(param); };

    UFUNCTION(BlueprintCallable, Category = ARS)
    bool HasValidPrimaryAttribute(FGameplayTag att) const { return AttributeSet.Attributes.Contains(att); };

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ARS)
    void OnComponentLoaded();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ARS)
    void OnComponentSaved();
};
