#pragma once

#include "ACFAIController.h"
#include "ACFAITypes.h"
#include "ACFCoreTypes.h"
#include "ACFStealthDetectionComponent.h"
#include "CoreMinimal.h"
#include "EliteAIIntelligenceComponent.h"
#include "Engine/TimerHandle.h"
#include "Game/ACFTypes.h"
#include "PortalDefenseAIController.generated.h"

// Forward Declarations
class APortalCore;
class UAIOverlordManager;
class UAILODManager;
class UACFActionsManagerComponent;
class ULightComponent;
class UPointLightComponent;
class USpotLightComponent;

USTRUCT(BlueprintType)
struct FPortalAIData {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float MovementSpeed = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float PatrolRadius = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float PlayerDetectionRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float AttackRange = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float AccuracyMultiplier = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    bool bUseAdvancedPathfinding = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    bool bCanFlank = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float AggressionLevel = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float ReactionTime = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    bool bUseACFActions = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Data")
    float PatrolSpeed = 0.5f;
};

UENUM(BlueprintType)
enum class EPatrolState : uint8 {
    Patrolling,
    ChasingPlayer,
    ReturningToPatrol,
    Investigating,
    InvestigatingSound
};

UENUM(BlueprintType)
enum class EDetectionType : uint8 {
    None,
    Visual,
    Audio,
    LightAggro
};

UCLASS()
class PORTAL_API APortalDefenseAIController : public AACFAIController {
    GENERATED_BODY()

public:
    APortalDefenseAIController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // LOD System Functions (Required for AI LOD Manager)
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdatePatrolLogic();

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateCombatBehavior();

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateTargeting();

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    bool IsInCombat() const;

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    bool IsEngagingPlayer() const;

    // Elite AI Integration
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetEliteMode(bool bEnabled, EEliteDifficultyLevel Difficulty = EEliteDifficultyLevel::Novice);

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    bool IsEliteModeActive() const;

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    EEliteDifficultyLevel GetEliteDifficulty() const;

    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetEliteDifficultyLevel(EEliteDifficultyLevel NewDifficulty);

    // Enhanced Combat Functions (Elite AI Integration)
    UFUNCTION(BlueprintCallable, Category = "Elite Combat")
    bool ShouldExecuteEliteDodge(const FVector& ThreatDirection, float ThreatSpeed);

    UFUNCTION(BlueprintCallable, Category = "Elite Combat")
    FVector GetEliteOptimalPosition(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Elite Combat")
    bool ShouldExecuteEliteAttack(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Elite Combat")
    void RecordPlayerCombatAction(const FString& ActionType, const FVector& ActionLocation);

    UFUNCTION(BlueprintCallable, Category = "Elite Combat")
    void ExecuteEliteTacticalPlan(APawn* Target);

    // Core AI Functions
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPortalTarget(APortalCore* NewPortalTarget);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPatrolCenter(FVector Center);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPatrolRadius(float Radius);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StartPatrolling();

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StopPatrolling();

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void ApplyAIUpgrade(const FPortalAIData& NewAIData);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void CheckForPlayerThreats();

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    FPortalAIData GetCurrentAIData() const { return CurrentAIData; }

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    EPatrolState GetCurrentPatrolState() const { return CurrentPatrolState; }

    // ACF Integration
    UFUNCTION(BlueprintCallable, Category = "ACF Combat")
    void TriggerAttackAction();

    UFUNCTION(BlueprintCallable, Category = "ACF Combat")
    void TriggerPatrolAction();

    UFUNCTION(BlueprintCallable, Category = "ACF Combat")
    void TriggerAlertAction();

    UFUNCTION(BlueprintCallable, Category = "ACF Combat")
    void SetCombatMode(bool bEnableCombat);

    UFUNCTION(BlueprintCallable, Category = "ACF Combat")
    void TriggerEliteAction(const FGameplayTag& ActionTag, EActionPriority Priority = EActionPriority::EMedium);

    // Advanced Combat Behaviors
    UFUNCTION(BlueprintCallable, Category = "Advanced Combat")
    void ExecuteFlankingManeuver(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Advanced Combat")
    void ExecuteTacticalRetreat();

    UFUNCTION(BlueprintCallable, Category = "Advanced Combat")
    void ExecuteAdvancedDodge(const FVector& ThreatDirection, const FVector& ThreatVelocity);

    UFUNCTION(BlueprintCallable, Category = "Advanced Combat")
    void ExecutePredictiveAttack(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Advanced Combat")
    void ExecuteCounterAttack(APawn* Target);

    // Overlord System Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void ReceiveOverlordCommand(const FString& Command, const TArray<FVector>& Parameters);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void ReportToOverlord(const FString& ReportType, const FVector& Location);

    // Detection and Investigation
    UFUNCTION(BlueprintCallable, Category = "Detection")
    void InvestigateLocation(FVector Location, float Duration = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "Detection")
    void InvestigateSound(FVector SoundLocation, float Duration = 8.0f);

    UFUNCTION(BlueprintCallable, Category = "Detection")
    void OnPlayerDetected(APawn* Player, EDetectionType DetectionType);

    UFUNCTION(BlueprintCallable, Category = "Detection")
    void OnPlayerLost();

    UFUNCTION(BlueprintCallable, Category = "Detection")
    void StartSoundInvestigation(FVector SoundLocation);

    // Environmental Interaction
    UFUNCTION(BlueprintCallable, Category = "Environment")
    void AnalyzeEnvironment();

    UFUNCTION(BlueprintCallable, Category = "Environment")
    FVector FindNearestCover(const FVector& ThreatDirection);

    UFUNCTION(BlueprintCallable, Category = "Environment")
    bool IsPositionInCover(const FVector& Position, const FVector& ThreatDirection);

    // Pattern Recognition and Learning
    UFUNCTION(BlueprintCallable, Category = "Learning")
    void AnalyzePlayerBehavior();

    UFUNCTION(BlueprintCallable, Category = "Learning")
    void AdaptCombatStrategy();

    UFUNCTION(BlueprintCallable, Category = "Learning")
    void RecordCombatOutcome(bool bVictorious, float CombatDuration);

protected:
    // Core Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Detection")
    TObjectPtr<UACFStealthDetectionComponent> StealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elite AI")
    TObjectPtr<UEliteAIIntelligenceComponent> EliteIntelligence;

    // AI Data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Settings")
    FPortalAIData BaseAIData;

    UPROPERTY(BlueprintReadOnly, Category = "AI Settings")
    FPortalAIData CurrentAIData;

    // Elite AI Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bEnableEliteMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI", meta = (EditCondition = "bEnableEliteMode"))
    EEliteDifficultyLevel EliteDifficulty = EEliteDifficultyLevel::Novice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bUseEliteInCombatOnly = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    float EliteActivationRange = 1500.0f;

    // Patrol Data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    FVector PatrolCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    FVector CurrentPatrolTarget = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    EPatrolState CurrentPatrolState;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    float PatrolAngle = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    bool bClockwisePatrol = true;

    // Detection Data
    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    TObjectPtr<APawn> DetectedPlayer;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    EDetectionType LastDetectionType;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    float PlayerDetectionTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    bool bPlayerSpottedInLight = false;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    bool bInvestigatingSound = false;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    float TimeSinceLastPlayerSighting = 0.0f;

    // Combat Data
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bIsEngagingPlayer = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float EngagementStartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float TotalEngagementTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float LastAttackTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 ConsecutiveHits = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 ConsecutiveMisses = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float LastDodgeTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FVector LastDodgeDirection = FVector::ZeroVector;

    // ACF Action Tags - Using Proper ACF Tags
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag AttackActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag PatrolActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag AlertActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag EquipWeaponActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag UnequipWeaponActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag DodgeActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag FlankActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag RetreatActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Actions")
    FGameplayTag CounterAttackActionTag;

    // Portal Defense
    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    TObjectPtr<APortalCore> PortalTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Defense")
    float MaxChaseDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Defense")
    float PlayerThreatMultiplier = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Defense")
    float InvestigationDuration = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Defense")
    float ReturnToPatrolDelay = 3.0f;

    // Overlord Integration
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TObjectPtr<UAIOverlordManager> OverlordManager;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TObjectPtr<UAILODManager> LODManager;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    int32 AIUnitID;

    // Environmental Awareness
    UPROPERTY(BlueprintReadOnly, Category = "Environment")
    TArray<FVector> KnownCoverPositions;

    UPROPERTY(BlueprintReadOnly, Category = "Environment")
    TArray<FVector> KnownFlankingPositions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float CoverDetectionRadius = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float EnvironmentAnalysisInterval = 5.0f;

    // Timers
    FTimerHandle PatrolUpdateTimer;
    FTimerHandle CombatUpdateTimer;
    FTimerHandle OverlordUpdateTimer;
    FTimerHandle InvestigationTimer;
    FTimerHandle SoundInvestigationTimer;
    FTimerHandle EnvironmentAnalysisTimer;
    FTimerHandle ReturnToPatrolTimer;

private:
    // Internal State
    bool bInCombatState = false;
    EAICombatState LastCombatState = EAICombatState::EIdle;
    float CombatStateChangeTime = 0.0f;
    float LastEnvironmentAnalysisTime = 0.0f;
    bool bEliteSystemsActive = false;

    // Combat Statistics
    int32 TotalCombatEncounters = 0;
    int32 SuccessfulCombatEncounters = 0;
    float AverageCombatDuration = 0.0f;
    float TotalCombatTime = 0.0f;

    // Internal Functions
    void FindPortalTarget();
    void SetupPortalDefense();
    void RegisterWithOverlord();
    void RegisterWithLODManager();
    void CalculateNextPatrolPoint();
    void UpdateCombatAccuracy(float EngagementTime);
    void UpdateCombatStatistics();
    UACFActionsManagerComponent* GetActionsManager() const;

    // Elite AI Internal Functions
    void InitializeEliteIntelligence();
    void UpdateEliteSystemsActivation();
    bool ShouldActivateEliteSystems() const;
    void ProcessEliteCombatDecision();
    void ExecuteEliteMovementStrategy(APawn* Target);
    void HandleEliteActionSelection(APawn* Target);

    // Advanced Combat Internal Functions
    void PerformAdvancedThreatAssessment();
    void ExecuteComplexManeuver(const FString& ManeuverType, APawn* Target);
    void UpdateCombatAdaptation();
    bool ValidateEliteAction(const FGameplayTag& ActionTag) const;

    // Timer Callbacks
    UFUNCTION()
    void OnPatrolUpdateTimer();

    UFUNCTION()
    void OnCombatUpdateTimer();

    UFUNCTION()
    void OnOverlordUpdateTimer();

    UFUNCTION()
    void OnInvestigationComplete();

    UFUNCTION()
    void OnSoundInvestigationComplete();

    UFUNCTION()
    void OnEnvironmentAnalysisTimer();

    UFUNCTION()
    void OnReturnToPatrolTimer();

    // Utility Functions
    float CalculateDistanceToPlayer() const;
    bool IsPlayerInRange(float Range) const;
    bool HasLineOfSightToPlayer() const;
    FVector GetPredictedPlayerPosition(float PredictionTime) const;
    bool IsPositionSafe(const FVector& Position) const;
    void CleanupExpiredTimers();
};