// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "ACFAITypes.h"
#include "ACFCoreTypes.h"
#include "ACFStealthDetectionComponent.h"
#include "CoreMinimal.h"
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
class UEliteAIIntelligenceComponent; // Forward declaration instead of include

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

// Forward declaration for enum that's in EliteAIIntelligenceComponent
enum class EEliteDifficultyLevel : uint8;

UCLASS()
class PORTAL_API APortalDefenseAIController : public AACFAIController {
    GENERATED_BODY()

public:
    APortalDefenseAIController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnPossess(APawn* InPawn) override;

public:
    // Core AI Management
    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void ActivateEliteMode(bool bActivate);

    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void SetEliteDifficultyLevel(EEliteDifficultyLevel NewDifficulty);

    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void UpdateAIData(const FPortalAIData& NewData);

    UFUNCTION(BlueprintPure, Category = "AI Management")
    FPortalAIData GetCurrentAIData() const { return CurrentAIData; }

    UFUNCTION(BlueprintPure, Category = "AI Management")
    bool IsEliteModeActive() const { return bEliteSystemsActive; }

    // Patrol Functions
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void StartPatrol(FVector CenterLocation, float Radius = 400.0f);

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void StopPatrol();

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void SetPatrolSpeed(float Speed);

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void ResumePatrol();

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void SetClockwisePatrol(bool bClockwise) { bClockwisePatrol = bClockwise; }

    // Combat Functions
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void EngageTarget(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void DisengageFromCombat();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ExecuteFlankingManeuver(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ExecuteTacticalRetreat();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ExecuteCounterAttack(APawn* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void UpdateCombatBehavior();

    // AI Overlord Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void ReceiveOverlordCommand(const FString& Command, const TArray<FVector>& Parameters);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void ReportToOverlord(const FString& ReportType, const FVector& Location);

    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    int32 GetAIUnitID() const { return AIUnitID; }

    // Detection Functions
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

    // Legacy Compatibility Functions (for existing code)
    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void ApplyAIUpgrade(const FPortalAIData& NewAIData);

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void SetPatrolCenter(const FVector& NewCenter);

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void SetPatrolRadius(float NewRadius);

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void StartPatrolling();

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void StopPatrolling();

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPortalTarget(APortalCore* NewTarget);

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
    EEliteDifficultyLevel EliteDifficulty;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bUseEliteInCombatOnly = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    float EliteActivationDistance = 2000.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Elite AI")
    bool bEliteSystemsActive = false;

    // Patrol State
    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    EPatrolState CurrentPatrolState = EPatrolState::Patrolling;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    FVector PatrolCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    FVector CurrentPatrolTarget = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    float PatrolAngle = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    bool bClockwisePatrol = true;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    bool bIsPatrolling = false;

    // Detection State
    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    TObjectPtr<APawn> DetectedPlayer;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    EDetectionType CurrentDetectionType = EDetectionType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    float TimeSinceLastDetection = 0.0f;

    // Investigation State
    UPROPERTY(BlueprintReadOnly, Category = "Investigation")
    FVector InvestigationTarget = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Investigation")
    float InvestigationTimeRemaining = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Investigation")
    bool bIsInvestigating = false;

    // Combat State
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    TObjectPtr<APawn> CombatTarget;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float CombatStartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 ConsecutiveHits = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 ConsecutiveMisses = 0;

    // Portal Defense
    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    TObjectPtr<APortalCore> PortalTarget;

    // AI Overlord Integration
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TObjectPtr<UAIOverlordManager> OverlordManager;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TObjectPtr<UAILODManager> LODManager;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    int32 AIUnitID = -1;

    // Timers
    FTimerHandle PatrolTimer;
    FTimerHandle InvestigationTimer;
    FTimerHandle EliteUpdateTimer;

private:
    // Private Helper Functions
    void InitializeEliteIntelligence();
    void UpdateEliteSystemsActivation();
    bool ShouldActivateEliteSystems() const;
    void ExecuteComplexManeuver(const FString& ManeuverType, APawn* Target);
    void UpdateCombatAdaptation();
    bool ValidateEliteAction(const FGameplayTag& ActionTag) const;
    void FindPortalTarget();
    void SetupPortalDefense();
    void RegisterWithOverlord();
    void RegisterWithLODManager();
    void CalculateNextPatrolPoint();
};