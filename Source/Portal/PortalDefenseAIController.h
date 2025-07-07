// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Engine/World.h"
#include "Game/ACFTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "PortalAITypes.h"
#include "UObject/ObjectPtr.h"
#include "PortalDefenseAIController.generated.h"

// Forward Declarations for ACF Ultimate Integration
class APortalCore;
class UAILODManager;
class UEliteAIIntelligenceComponent;
class UACFStealthDetectionComponent;
class UACFCombatBehaviourComponent;
class UAIBatchProcessor;
class UAIOverseenComponent;

/**
 * Portal Defense AI Controller
 *
 * Advanced AI controller designed for Portal Defense scenarios with deep ACF Ultimate integration.
 * Features intelligent patrol behavior, dynamic LOD management, elite AI capabilities,
 * and seamless combat system coordination through ACF framework components.
 *
 * Key Features:
 * - Dynamic patrol pattern generation around portal objectives
 * - Intelligent player detection and engagement using ACF stealth detection
 * - Elite AI mode with advanced tactical behaviors
 * - Performance-optimized LOD integration for large-scale battles
 * - ACF Ultimate combat behavior integration for sophisticated combat AI
 * - Tactical coordination with other AI controllers
 * - Threat assessment and response systems
 */
UCLASS(BlueprintType, meta = (DisplayName = "Portal Defense AI Controller"))
class PORTAL_API APortalDefenseAIController : public AACFAIController {
    GENERATED_BODY()

public:
    APortalDefenseAIController();

protected:
    // ============================================================================
    // CORE UNREAL ENGINE LIFECYCLE
    // ============================================================================
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ============================================================================
    // CORE PORTAL DEFENSE FUNCTIONS
    // ============================================================================

    /** Set the portal target this AI should defend */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPortalTarget(APortalCore* NewTarget);

    /** Start patrolling around a specific location with defined radius */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense", meta = (DisplayName = "Start Patrolling With Parameters"))
    void StartPatrollingAtLocation(FVector Center, float Radius = 500.0f);

    /** Start patrolling with current configuration */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense", meta = (DisplayName = "Start Patrolling"))
    void BeginPatrolling();

    /** Investigate a specific location */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void InvestigateLocation(FVector Location);

    /** Handle player detection event */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void OnPlayerDetected(APawn* Player);

    /** Handle player lost event */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void OnPlayerLost();

    // ============================================================================
    // AI BEHAVIOR AND STATE MANAGEMENT
    // ============================================================================

    /** Update AI behavior based on LOD level */
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateAIBehavior(int32 LODLevel);

    /** Update patrol logic */
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdatePatrolLogic();

    /** Update combat behavior */
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateCombatBehavior();

    /** Update targeting system */
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateTargeting();

    /** Transition to a new AI state */
    UFUNCTION(BlueprintCallable, Category = "AI State")
    void TransitionToState(EPortalAIState NewState);

    // ============================================================================
    // LOD SYSTEM INTEGRATION
    // ============================================================================

    /** Set AI update frequency for LOD optimization */
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void SetAIUpdateFrequency(float UpdateFrequency);

    /** Set behavior complexity multiplier */
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void SetBehaviorComplexity(float ComplexityMultiplier);

    /** Set current LOD level */
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void SetCurrentLODLevel(int32 LODLevel);

    // ============================================================================
    // TACTICAL COORDINATION
    // ============================================================================

    /** Set defensive position for tactical coordination */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void SetDefensivePosition(const FVector& Position);

    /** Set AI behavior mode */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void SetAIBehaviorMode(EAIBehaviorMode NewMode);

    /** Enable or disable coordination */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void SetCoordinationEnabled(bool bEnabled);

    /** Set threat level for tactical response */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void SetThreatLevel(EThreatLevel ThreatLevel);

    // ============================================================================
    // ELITE AI FUNCTIONS
    // ============================================================================

    /** Activate elite AI mode */
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void ActivateEliteMode();

    /** Deactivate elite AI mode */
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void DeactivateEliteMode();

    /** Check if elite mode is active */
    UFUNCTION(BlueprintPure, Category = "Elite AI")
    bool IsEliteModeActive() const { return bEnableEliteMode; }

    /** Set elite activation distance */
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetEliteActivationDistance(float Distance) { EliteActivationDistance = Distance; }

    // ============================================================================
    // QUERY FUNCTIONS
    // ============================================================================

    /** Check if AI is currently in combat */
    UFUNCTION(BlueprintPure, Category = "AI State")
    bool IsInCombat() const;

    /** Check if AI is engaging player */
    UFUNCTION(BlueprintPure, Category = "AI State")
    bool IsEngagingPlayer() const;

    /** Get current AI state */
    UFUNCTION(BlueprintPure, Category = "AI State")
    EPortalAIState GetCurrentState() const { return CurrentState; }

    /** Get current AI data */
    UFUNCTION(BlueprintPure, Category = "AI Data")
    FPortalAIData GetCurrentAIData() const { return CurrentAIData; }

    /** Set AI data */
    UFUNCTION(BlueprintCallable, Category = "AI Data")
    void SetAIData(const FPortalAIData& NewData) { CurrentAIData = NewData; }

    /** Get array of managed AI controllers (Blueprint-safe) */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    TArray<APortalDefenseAIController*> GetManagedAIControllers() const;

    // ============================================================================
    // CONFIGURATION PROPERTIES
    // ============================================================================

    /** Portal AI configuration settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    FPortalAIConfig AIConfig;

    /** Elite AI activation distance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float EliteActivationDistance = 800.0f;

    /** Enable elite AI mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bEnableEliteMode = false;

protected:
    // ============================================================================
    // INTERNAL STATE VARIABLES
    // ============================================================================

    /** Current AI state */
    UPROPERTY()
    EPortalAIState CurrentState = EPortalAIState::Patrolling;

    /** Current AI data structure */
    UPROPERTY()
    FPortalAIData CurrentAIData;

    /** Portal target this AI is defending */
    UPROPERTY()
    TObjectPtr<APortalCore> PortalTarget;

    /** Currently detected player */
    UPROPERTY()
    TObjectPtr<APawn> DetectedPlayer;

    /** Patrol center position */
    UPROPERTY()
    FVector PatrolCenter = FVector::ZeroVector;

    /** Last known player location */
    UPROPERTY()
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    /** Array of patrol points */
    UPROPERTY()
    TArray<FVector> PatrolPoints;

    /** Current patrol point index */
    UPROPERTY()
    int32 CurrentPatrolIndex = 0;

    // ============================================================================
    // LOD AND PERFORMANCE VARIABLES
    // ============================================================================

    /** Current AI update frequency for LOD optimization */
    UPROPERTY()
    float AIUpdateFrequency = 10.0f;

    /** Current behavior complexity multiplier */
    UPROPERTY()
    float BehaviorComplexityMultiplier = 1.0f;

    /** Current LOD level assigned by LOD Manager */
    UPROPERTY()
    int32 CurrentLODLevel = 1;

    // ============================================================================
    // COORDINATION VARIABLES
    // ============================================================================

    /** Current defensive position for tactical coordination */
    UPROPERTY()
    FVector DefensivePosition = FVector::ZeroVector;

    /** Whether this AI has a defensive position assigned */
    UPROPERTY()
    bool bHasDefensivePosition = false;

    /** Current AI behavior mode */
    UPROPERTY()
    EAIBehaviorMode CurrentBehaviorMode = EAIBehaviorMode::Patrol;

    /** Whether coordination with other AI is enabled */
    UPROPERTY()
    bool bCoordinationEnabled = true;

    /** Current threat level assessment */
    UPROPERTY()
    EThreatLevel CurrentThreatLevel = EThreatLevel::Low;

    // ============================================================================
    // COMPONENT REFERENCES
    // ============================================================================

    /** Elite AI Intelligence Component for advanced behaviors */
    UPROPERTY()
    TObjectPtr<UEliteAIIntelligenceComponent> EliteIntelligence;

    /** ACF Stealth Detection Component for player detection */
    UPROPERTY()
    TObjectPtr<UACFStealthDetectionComponent> StealthDetection;

    /** LOD Manager reference for performance optimization */
    UPROPERTY()
    TObjectPtr<UAILODManager> LODManager;

    // ============================================================================
    // TIMER HANDLES
    // ============================================================================

    /** Timer for patrol movement */
    UPROPERTY()
    FTimerHandle PatrolTimer;

    /** Timer for investigation duration */
    UPROPERTY()
    FTimerHandle InvestigationTimer;

    /** Timer for elite system updates */
    UPROPERTY()
    FTimerHandle EliteUpdateTimer;

    // ============================================================================
    // INTERNAL PROCESSING FUNCTIONS (Non-UFUNCTION)
    // ============================================================================

    /** Initialize AI components and systems */
    void InitializeComponents();

    /** Register with AI management systems */
    void RegisterWithManagers();

    /** Setup ACF integration */
    void SetupACFIntegration();

    /** Configure ACF combat behavior */
    void ConfigureCombatBehavior();

    /** Generate patrol points around center */
    void GeneratePatrolPoints();

    /** Move to next patrol point */
    void MoveToNextPatrolPoint();

    /** Handle patrol point reached */
    void OnPatrolPointReached();

    /** Handle state transition */
    void HandleStateTransition(EPortalAIState FromState, EPortalAIState ToState);

    /** Update combat state */
    void UpdateCombatState();

    /** Update elite systems */
    void UpdateEliteSystemsActivation();

    /** Configure elite capabilities */
    void ConfigureEliteCapabilities();

    /** Optimize for current LOD */
    void OptimizeForCurrentLOD();

    /** Update performance metrics */
    void UpdatePerformanceMetrics();

    /** Apply behavior mode changes */
    void ApplyBehaviorModeChanges(EAIBehaviorMode OldMode, EAIBehaviorMode NewMode);

    /** Register with coordination systems */
    void RegisterWithCoordinationSystems();

    /** Unregister from coordination systems */
    void UnregisterFromCoordinationSystems();

    /** Should engage in combat */
    bool ShouldEngageInCombat() const;

    /** Start patrolling implementation (internal use) */
    void StartPatrolling(FVector Center, float Radius = 500.0f);

    // ============================================================================
    // MOVEMENT AND PATHFINDING
    // ============================================================================

    /** Move completed callback */
    UFUNCTION()
    void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);
};