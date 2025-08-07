// PortalAILearningComponent.h
#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalAILearningComponent.generated.h"

// Forward declarations
class APortalACFAIController;

/**
 * Combat pattern data structure for learning
 */
USTRUCT(BlueprintType)
struct FCombatPattern {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName PatternName;

    UPROPERTY(BlueprintReadOnly)
    int32 TimesObserved = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 SuccessfulCounters = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageReactionTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float SuccessRate = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> CommonPositions;

    FCombatPattern()
    {
        PatternName = NAME_None;
        TimesObserved = 0;
        SuccessfulCounters = 0;
        AverageReactionTime = 0.0f;
        SuccessRate = 0.0f;
    }
};

/**
 * Player behavior profile for adaptive AI
 */
USTRUCT(BlueprintType)
struct FPlayerBehaviorProfile {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString PlayerID;

    UPROPERTY(BlueprintReadOnly)
    float PreferredCombatRange = 500.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageMovementSpeed = 300.0f;

    UPROPERTY(BlueprintReadOnly)
    float DodgeFrequency = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float BlockFrequency = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AttackFrequency = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TMap<FName, FCombatPattern> KnownPatterns;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> PreferredWeapons;

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> FrequentPositions;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalEncounters = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerVictories = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 AIVictories = 0;

    FPlayerBehaviorProfile()
    {
        PlayerID = "";
        PreferredCombatRange = 500.0f;
        AverageMovementSpeed = 300.0f;
        DodgeFrequency = 0.0f;
        BlockFrequency = 0.0f;
        AttackFrequency = 0.0f;
        TotalEncounters = 0;
        PlayerVictories = 0;
        AIVictories = 0;
    }
};

/**
 * Learning session data
 */
USTRUCT(BlueprintType)
struct FLearningSessionData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float SessionStartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float SessionDuration = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 ActionsObserved = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PatternsLearned = 0;

    UPROPERTY(BlueprintReadOnly)
    float AdaptationScore = 0.0f;

    FLearningSessionData()
    {
        SessionStartTime = 0.0f;
        SessionDuration = 0.0f;
        ActionsObserved = 0;
        PatternsLearned = 0;
        AdaptationScore = 0.0f;
    }
};

/**
 * AI Learning Component for Portal AI
 *
 * Implements adaptive AI that learns from player behavior
 * Stores and analyzes combat patterns for improved responses
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UPortalAILearningComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UPortalAILearningComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    bool bEnableLearning = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    bool bPersistLearningData = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    float LearningRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    int32 MinObservationsForPattern = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    float PatternRecognitionThreshold = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    float MemoryRetentionTime = 300.0f; // 5 minutes

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    int32 MaxStoredPatterns = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Learning Config")
    int32 MaxPositionHistory = 50;

    // Current learning state
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning State")
    FPlayerBehaviorProfile CurrentPlayerProfile;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning State")
    FLearningSessionData CurrentSession;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning State")
    float AdaptationLevel = 0.0f; // 0-1 value

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning State")
    bool bIsLearning = false;

    // Shared knowledge database (static for all AI)
    static TMap<FString, FPlayerBehaviorProfile> GlobalPlayerDatabase;

public:
    // Initialization
    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void Initialize(APortalACFAIController* Controller);

    // Learning control
    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void StartLearning();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void StopLearning();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void ResetLearning();

    // Combat learning
    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void ProcessCombatData(AActor* Target, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void RecordPlayerAction(FName ActionType, const FVector& ActionLocation);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void RecordCombatOutcome(bool bPlayerWon);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void OnCombatStarted(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void OnCombatEnded();

    // Pattern recognition
    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    bool DetectPattern(const TArray<FName>& RecentActions, FCombatPattern& OutPattern);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    FCombatPattern GetKnownPattern(FName PatternName) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    TArray<FCombatPattern> GetAllKnownPatterns() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    float GetPatternPrediction(FName PatternName) const;

    // Adaptive responses
    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    FName GetCounterAction(FName PlayerAction) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    FVector PredictPlayerPosition(float TimeInFuture) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    float GetOptimalCombatRange() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    float GetAdaptedReactionTime() const;

    // Data persistence
    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void SaveLearningData();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void LoadLearningData();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    void ShareKnowledge(UPortalAILearningComponent* OtherAI);

    // Queries
    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    float GetAdaptationLevel() const { return AdaptationLevel; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    FPlayerBehaviorProfile GetPlayerProfile() const { return CurrentPlayerProfile; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI Learning")
    bool IsLearning() const { return bIsLearning; }

    // Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Learning")
    void OnPatternRecognized(const FCombatPattern& Pattern);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Learning")
    void OnAdaptationLevelChanged(float NewLevel);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Learning")
    void OnPlayerProfileUpdated(const FPlayerBehaviorProfile& Profile);

private:
    UPROPERTY()
    APortalACFAIController* OwnerController;

    // Internal tracking
    TArray<FName> RecentPlayerActions;
    TArray<FVector> RecentPlayerPositions;
    TMap<FName, float> ActionTimestamps;
    float LastActionTime = 0.0f;
    float CombatStartTime = 0.0f;

    // Learning metrics
    int32 CorrectPredictions = 0;
    int32 TotalPredictions = 0;
    float PredictionAccuracy = 0.0f;

    // Internal methods
    void UpdatePlayerProfile(AActor* Player, float DeltaTime);
    void AnalyzeMovementPattern(const FVector& PlayerLocation);
    void UpdateActionFrequencies(FName ActionType);
    void ProcessPatternRecognition();
    void CalculateAdaptationLevel();
    void CleanupOldData();
    void ApplyLearningToController();
    FString GetPlayerID(AActor* Player) const;
};