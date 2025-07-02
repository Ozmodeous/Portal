// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "ACFAITypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Game/ACFTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "PortalDefenseAIController.h"
#include "EliteAIIntelligenceComponent.generated.h"

UENUM(BlueprintType)
enum class EEliteDifficultyLevel : uint8 {
    Disabled = 0,
    Novice = 1,
    Skilled = 2,
    Veteran = 3,
    Expert = 4,
    Master = 5,
    Grandmaster = 6,
    Legend = 7,
    Nightmare = 8,
    Impossible = 9,
    Godlike = 10
};

UENUM(BlueprintType)
enum class EEliteIntelligenceMode : uint8 {
    Reactive, // Responds to immediate threats
    Predictive, // Anticipates player actions
    Adaptive, // Learns and counters player patterns
    Strategic, // Plans multi-step tactics
    Psychological // Manipulates player expectations
};

USTRUCT(BlueprintType)
struct FPlayerBehaviorPattern {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    TArray<FVector> RecentPositions;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    TArray<FVector> AttackPositions;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    TArray<float> AttackTimings;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    TArray<FVector> DodgeDirections;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    float AverageMovementSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    float PreferredEngagementDistance = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    FVector PreferredDodgeDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    float AttackFrequency = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    bool bPrefersCircleStrafing = false;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    bool bUsesEnvironmentCover = false;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    float PatternConfidence = 0.0f;

    FPlayerBehaviorPattern()
    {
        RecentPositions.Reserve(50);
        AttackPositions.Reserve(20);
        AttackTimings.Reserve(20);
        DodgeDirections.Reserve(30);
        AverageMovementSpeed = 0.0f;
        PreferredEngagementDistance = 0.0f;
        PreferredDodgeDirection = FVector::ZeroVector;
        AttackFrequency = 0.0f;
        bPrefersCircleStrafing = false;
        bUsesEnvironmentCover = false;
        PatternConfidence = 0.0f;
    }
};

USTRUCT(BlueprintType)
struct FEliteTacticalPlan {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Plan")
    TArray<FVector> PlannedPositions;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Plan")
    TArray<FGameplayTag> PlannedActions;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Plan")
    TArray<float> ActionTimings;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Plan")
    FVector PredictedPlayerPosition;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Plan")
    float PlanConfidence = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Plan")
    float ExecutionStartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Plan")
    bool bIsExecuting = false;

    FEliteTacticalPlan()
    {
        PlannedPositions.Reserve(10);
        PlannedActions.Reserve(10);
        ActionTimings.Reserve(10);
        PredictedPlayerPosition = FVector::ZeroVector;
        PlanConfidence = 0.0f;
        ExecutionStartTime = 0.0f;
        bIsExecuting = false;
    }
};

USTRUCT(BlueprintType)
struct FEliteDifficultySettings {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    EEliteIntelligenceMode IntelligenceMode = EEliteIntelligenceMode::Reactive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float ReactionTimeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float PredictionAccuracy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float PatternLearningSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float TacticalPlanningDepth = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float DodgePerfection = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float AttackPrediction = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float FlankingIntelligence = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    float EnvironmentUsage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    bool bCanCounterAdapt = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    bool bUsesFramePerfectTiming = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Settings")
    bool bCanManipulatePlayer = false;

    FEliteDifficultySettings()
    {
        IntelligenceMode = EEliteIntelligenceMode::Reactive;
        ReactionTimeMultiplier = 1.0f;
        PredictionAccuracy = 0.0f;
        PatternLearningSpeed = 0.0f;
        TacticalPlanningDepth = 1.0f;
        DodgePerfection = 0.0f;
        AttackPrediction = 0.0f;
        FlankingIntelligence = 0.0f;
        EnvironmentUsage = 0.0f;
        bCanCounterAdapt = false;
        bUsesFramePerfectTiming = false;
        bCanManipulatePlayer = false;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEliteBehaviorTriggered, FString, BehaviorName, float, Intensity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerPatternDetected, FPlayerBehaviorPattern, DetectedPattern);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalPlanExecuted, FEliteTacticalPlan, ExecutedPlan);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UEliteAIIntelligenceComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UEliteAIIntelligenceComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Core Elite AI Control
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetEliteMode(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetDifficultyLevel(EEliteDifficultyLevel NewDifficulty);

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    bool IsEliteModeEnabled() const { return bEliteModeEnabled; }

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    EEliteDifficultyLevel GetCurrentDifficulty() const { return CurrentDifficulty; }

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    FEliteDifficultySettings GetCurrentSettings() const { return CurrentSettings; }

    // Combat Intelligence
    UFUNCTION(BlueprintCallable, Category = "Elite AI | Combat")
    bool ShouldDodgeNow(const FVector& ThreatDirection, float ThreatSpeed);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Combat")
    FVector GetOptimalDodgeDirection(const FVector& ThreatDirection);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Combat")
    bool ShouldAttackNow(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Combat")
    FVector PredictPlayerPosition(float PredictionTime);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Combat")
    FGameplayTag GetOptimalAttackAction(APawn* Target);

    // Tactical Intelligence
    UFUNCTION(BlueprintCallable, Category = "Elite AI | Tactics")
    FVector GetOptimalFlankingPosition(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Tactics")
    bool ShouldExecuteTacticalRetreat();

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Tactics")
    FEliteTacticalPlan GenerateTacticalPlan(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Tactics")
    void ExecuteTacticalPlan(const FEliteTacticalPlan& Plan);

    // Learning and Adaptation
    UFUNCTION(BlueprintCallable, Category = "Elite AI | Learning")
    void RecordPlayerAction(APawn* Player, const FVector& ActionPosition, const FString& ActionType);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Learning")
    void AnalyzePlayerPatterns();

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Learning")
    void AdaptToPlayerBehavior();

    UFUNCTION(BlueprintPure, Category = "Elite AI | Learning")
    FPlayerBehaviorPattern GetCurrentPlayerPattern() const { return CurrentPlayerPattern; }

    // Psychological Warfare
    UFUNCTION(BlueprintCallable, Category = "Elite AI | Psychology")
    void ExecutePsychologicalTactic(const FString& TacticName);

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Psychology")
    bool ShouldBaitPlayer();

    UFUNCTION(BlueprintCallable, Category = "Elite AI | Psychology")
    FVector GetPlayerBaitPosition();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Elite AI")
    FOnEliteBehaviorTriggered OnEliteBehaviorTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Elite AI")
    FOnPlayerPatternDetected OnPlayerPatternDetected;

    UPROPERTY(BlueprintAssignable, Category = "Elite AI")
    FOnTacticalPlanExecuted OnTacticalPlanExecuted;

protected:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bEliteModeEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    EEliteDifficultyLevel CurrentDifficulty = EEliteDifficultyLevel::Disabled;

    UPROPERTY(BlueprintReadOnly, Category = "Elite AI")
    FEliteDifficultySettings CurrentSettings;

    // Pattern Recognition
    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    FPlayerBehaviorPattern CurrentPlayerPattern;

    UPROPERTY(BlueprintReadOnly, Category = "Pattern Analysis")
    TArray<FPlayerBehaviorPattern> HistoricalPatterns;

    // Tactical Planning
    UPROPERTY(BlueprintReadOnly, Category = "Tactical Planning")
    FEliteTacticalPlan CurrentTacticalPlan;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical Planning")
    TArray<FEliteTacticalPlan> TacticalPlanHistory;

    // References
    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<AACFAIController> OwnerAIController;

    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<APawn> TrackedPlayer;

    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<APawn> OwnerPawn;

private:
    // Internal State
    float LastAnalysisTime = 0.0f;
    float LastPredictionTime = 0.0f;
    float LastTacticalPlanTime = 0.0f;
    float CombatStartTime = 0.0f;
    bool bInCombat = false;

    // Frame-perfect timing data
    TArray<float> RecentFrameTimes;
    float AverageFrameTime = 16.67f;

    // Prediction caching
    FVector CachedPlayerPositionPrediction = FVector::ZeroVector;
    float PredictionCacheTime = 0.0f;
    float PredictionCacheValidTime = 0.1f;

    // Internal Methods
    void InitializeDifficultySettings();
    void UpdateDifficultySettings();
    void UpdatePlayerTracking();
    void UpdateCombatState();
    void UpdateFrameTiming(float DeltaTime);

    FEliteDifficultySettings GetSettingsForDifficulty(EEliteDifficultyLevel Difficulty) const;
    void ProcessPlayerMovement(APawn* Player);
    void ProcessPlayerCombatAction(APawn* Player, const FVector& ActionPosition, const FString& ActionType);

    bool CanPredictWithAccuracy(float RequiredAccuracy) const;
    bool IsFramePerfectTimingRequired() const;
    float GetCurrentReactionTime() const;

    FVector CalculateAdvancedDodge(const FVector& ThreatDirection, const FVector& ThreatVelocity);
    FVector CalculateInterceptPosition(APawn* Target, float ProjectileSpeed);
    FVector CalculateFlankingRoute(APawn* Target, float OptimalDistance);

    void ExecuteCounterAdaptation();
    void UpdatePsychologicalProfile();
    bool ShouldUsePsychologicalWarfare() const;

    static TMap<EEliteDifficultyLevel, FEliteDifficultySettings> DifficultyPresets;
};