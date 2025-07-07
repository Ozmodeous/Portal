// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Game/ACFTypes.h"
#include "GameplayTagContainer.h"
#include "EliteAIIntelligenceComponent.generated.h"

// Forward Declarations
class AACFAIController;
class APawn;
class UACFCombatBehaviourComponent;

UENUM(BlueprintType)
enum class EEliteDifficultyLevel : uint8 {
    Disabled = 0,
    Novice = 1,
    Skilled = 2,
    Veteran = 3,
    Expert = 4,
    Master = 5,
    Grandmaster = 6
};

USTRUCT(BlueprintType)
struct FPlayerBehaviorPattern {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> RecentPositions;

    UPROPERTY(BlueprintReadOnly)
    TArray<float> AttackTimings;

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> DodgeDirections;

    UPROPERTY(BlueprintReadOnly)
    float AverageMovementSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PreferredEngagementDistance = 600.0f;

    UPROPERTY(BlueprintReadOnly)
    FVector PreferredDodgeDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    bool bPrefersCircleStrafing = false;

    UPROPERTY(BlueprintReadOnly)
    float PredictabilityScore = 0.5f;
};

USTRUCT(BlueprintType)
struct FEliteSettings {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReactionTimeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PredictionAccuracy = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DodgePerfection = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FlankingIntelligence = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanCounterAdapt = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUsesFramePerfectTiming = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEliteBehaviorTriggered, FString, BehaviorName, float, Intensity);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UEliteAIIntelligenceComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UEliteAIIntelligenceComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Core Elite AI Functions
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetEliteMode(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetDifficultyLevel(EEliteDifficultyLevel NewDifficulty);

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    bool IsEliteModeEnabled() const { return bEliteModeEnabled; }

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    EEliteDifficultyLevel GetCurrentDifficulty() const { return CurrentDifficulty; }

    // Player Analysis
    UFUNCTION(BlueprintCallable, Category = "Player Analysis")
    void RecordPlayerAction(APawn* Player, const FVector& ActionLocation, const FString& ActionType);

    UFUNCTION(BlueprintPure, Category = "Player Analysis")
    FPlayerBehaviorPattern GetCurrentPlayerPattern() const { return CurrentPlayerPattern; }

    // ACF Integration - Enhanced Combat Intelligence
    UFUNCTION(BlueprintCallable, Category = "ACF Elite")
    EAICombatState GetOptimalCombatState(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "ACF Elite")
    FGameplayTag GetOptimalACFAction(EAICombatState CombatState);

    UFUNCTION(BlueprintCallable, Category = "ACF Elite")
    bool ShouldDodgeNow(const FVector& ThreatDirection, float ThreatSpeed);

    UFUNCTION(BlueprintCallable, Category = "ACF Elite")
    FVector PredictPlayerPosition(float PredictionTime);

    UFUNCTION(BlueprintCallable, Category = "ACF Elite")
    bool ShouldCounterAttack();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEliteBehaviorTriggered OnEliteBehaviorTriggered;

protected:
    // Core Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bEliteModeEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    EEliteDifficultyLevel CurrentDifficulty = EEliteDifficultyLevel::Novice;

    UPROPERTY(BlueprintReadOnly, Category = "Elite AI")
    FEliteSettings CurrentSettings;

    // Player Analysis Data
    UPROPERTY(BlueprintReadOnly, Category = "Player Analysis")
    FPlayerBehaviorPattern CurrentPlayerPattern;

    // References
    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<AACFAIController> OwnerAIController = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<APawn> TrackedPlayer = nullptr;

private:
    // Internal State
    float LastAnalysisTime = 0.0f;
    float CombatStartTime = 0.0f;
    TArray<float> RecentFrameTimes;
    float AverageFrameTime = 16.67f;

    // Internal Methods
    void UpdateDifficultySettings();
    void AnalyzePlayerBehavior();
    void UpdatePlayerTracking();
    FEliteSettings GetSettingsForDifficulty(EEliteDifficultyLevel Difficulty) const;
    bool DetectCircleStrafePattern() const;
    float CalculatePredictabilityScore() const;
    void UpdateFrameTiming(float DeltaTime);
};