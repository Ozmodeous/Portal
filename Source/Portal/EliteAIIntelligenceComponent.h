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
#include "EliteAIIntelligenceComponent.generated.h"

// Forward Declarations
class APortalDefenseAIController; // Forward declaration instead of include

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
struct FEliteAISettings {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    float ReactionTimeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    float AccuracyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    float PredictionAccuracy = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    float AdaptationSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    bool bCanPredictMovement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    bool bCanCounterAdapt = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    bool bUsePsychologicalWarfare = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite Settings")
    float PatternAnalysisDepth = 10.0f;

    FEliteAISettings()
    {
        ReactionTimeMultiplier = 1.0f;
        AccuracyMultiplier = 1.0f;
        PredictionAccuracy = 0.5f;
        AdaptationSpeed = 1.0f;
        bCanPredictMovement = true;
        bCanCounterAdapt = false;
        bUsePsychologicalWarfare = false;
        PatternAnalysisDepth = 10.0f;
    }
};

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
    void SetDifficultyLevel(EEliteDifficultyLevel NewLevel);

    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetIntelligenceMode(EEliteIntelligenceMode NewMode);

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    bool IsEliteModeEnabled() const { return bEliteModeEnabled; }

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    EEliteDifficultyLevel GetCurrentDifficultyLevel() const { return CurrentDifficultyLevel; }

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    EEliteIntelligenceMode GetCurrentIntelligenceMode() const { return CurrentIntelligenceMode; }

    // Player Analysis Functions
    UFUNCTION(BlueprintCallable, Category = "Player Analysis")
    void StartPlayerTracking(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Player Analysis")
    void StopPlayerTracking();

    UFUNCTION(BlueprintCallable, Category = "Player Analysis")
    void RecordPlayerPosition(const FVector& Position);

    UFUNCTION(BlueprintCallable, Category = "Player Analysis")
    void RecordPlayerAttack(const FVector& AttackPosition, float Timing);

    UFUNCTION(BlueprintCallable, Category = "Player Analysis")
    void RecordPlayerDodge(const FVector& DodgeDirection);

    UFUNCTION(BlueprintPure, Category = "Player Analysis")
    FPlayerBehaviorPattern GetCurrentPlayerPattern() const { return CurrentPlayerPattern; }

    // Prediction Functions
    UFUNCTION(BlueprintCallable, Category = "Prediction")
    FVector PredictPlayerPosition(float PredictionTime = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Prediction")
    FVector PredictPlayerMovement(float TimeAhead = 0.5f);

    UFUNCTION(BlueprintCallable, Category = "Prediction")
    bool ShouldPredictDodge(const FVector& AttackDirection);

    UFUNCTION(BlueprintPure, Category = "Prediction")
    float GetPredictionConfidence() const { return PredictionConfidence; }

    // Adaptation Functions
    UFUNCTION(BlueprintCallable, Category = "Adaptation")
    void AdaptToPlayerBehavior();

    UFUNCTION(BlueprintCallable, Category = "Adaptation")
    void CounterPlayerStrategy();

    UFUNCTION(BlueprintCallable, Category = "Adaptation")
    FString GetRecommendedTactic();

    UFUNCTION(BlueprintCallable, Category = "Adaptation")
    void RecordTacticSuccess(const FString& TacticName, bool bSuccessful);

    // Combat Intelligence
    UFUNCTION(BlueprintCallable, Category = "Combat Intelligence")
    float CalculateOptimalEngagementDistance();

    UFUNCTION(BlueprintCallable, Category = "Combat Intelligence")
    FVector CalculateOptimalAttackPosition(const FVector& PlayerPosition);

    UFUNCTION(BlueprintCallable, Category = "Combat Intelligence")
    bool ShouldUseFlankingManeuver();

    UFUNCTION(BlueprintCallable, Category = "Combat Intelligence")
    bool ShouldRetreat();

    // Psychological Warfare
    UFUNCTION(BlueprintCallable, Category = "Psychological Warfare")
    void ExecutePsychologicalTactic(const FString& TacticName);

    UFUNCTION(BlueprintCallable, Category = "Psychological Warfare")
    void CreateFalsePattern();

    UFUNCTION(BlueprintCallable, Category = "Psychological Warfare")
    void ExecuteFeint();

    // Settings and Configuration
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void UpdateEliteSettings();

    UFUNCTION(BlueprintPure, Category = "Configuration")
    FEliteAISettings GetCurrentSettings() const { return CurrentSettings; }

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void ResetBehaviorPatterns();

protected:
    // Core Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bEliteModeEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    EEliteDifficultyLevel CurrentDifficultyLevel = EEliteDifficultyLevel::Novice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    EEliteIntelligenceMode CurrentIntelligenceMode = EEliteIntelligenceMode::Reactive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    FEliteAISettings CurrentSettings;

    // Player Tracking
    UPROPERTY(BlueprintReadOnly, Category = "Player Tracking")
    TObjectPtr<APawn> TrackedPlayer;

    UPROPERTY(BlueprintReadOnly, Category = "Player Tracking")
    FPlayerBehaviorPattern CurrentPlayerPattern;

    UPROPERTY(BlueprintReadOnly, Category = "Player Tracking")
    float TrackingStartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Player Tracking")
    bool bIsTrackingPlayer = false;

    // Prediction Data
    UPROPERTY(BlueprintReadOnly, Category = "Prediction")
    FVector CachedPlayerPositionPrediction = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Prediction")
    float PredictionConfidence = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Prediction")
    float LastPredictionTime = 0.0f;

    // Adaptation Data
    UPROPERTY(BlueprintReadOnly, Category = "Adaptation")
    TMap<FString, float> TacticSuccessRates;

    UPROPERTY(BlueprintReadOnly, Category = "Adaptation")
    TArray<FString> RecentTactics;

    UPROPERTY(BlueprintReadOnly, Category = "Adaptation")
    float AdaptationCooldown = 0.0f;

    // Owner Reference
    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<APortalDefenseAIController> OwnerController;

    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<APawn> OwnerPawn;

private:
    // Internal Update Functions
    void UpdatePlayerPattern();
    void UpdatePredictions();
    void UpdateAdaptation();
    void AnalyzePlayerMovement();
    void AnalyzePlayerCombat();
    void ApplyDifficultySettings();
    void CachePredictions();
    bool IsPlayerBehaviorConsistent() const;
    float CalculatePatternConfidence() const;
};