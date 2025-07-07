// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Game/ACFTypes.h"
#include "PortalDefenseAIController.generated.h"

// Forward Declarations for ACF Ultimate Integration
class APortalCore;
class UAILODManager;
class UEliteAIIntelligenceComponent;
class UACFStealthDetectionComponent;
class UACFCombatBehaviourComponent;

/**
 * Portal AI State Enumeration
 * Defines the behavioral states for Portal Defense AI Controllers
 * Integrates seamlessly with ACF Ultimate's AI state management system
 */
UENUM(BlueprintType)
enum class EPortalAIState : uint8 {
    Patrolling UMETA(DisplayName = "Patrolling"),
    Investigating UMETA(DisplayName = "Investigating"),
    ChasingPlayer UMETA(DisplayName = "Chasing Player"),
    Guarding UMETA(DisplayName = "Guarding Portal"),
    Returning UMETA(DisplayName = "Returning to Patrol")
};

/**
 * Portal AI Configuration Structure
 * Comprehensive configuration data for Portal Defense AI behavior
 * Optimized for ACF Ultimate combat system integration
 */
USTRUCT(BlueprintType)
struct PORTAL_API FPortalAIConfig {
    GENERATED_BODY()

    /** Base patrol radius around the assigned portal */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float PatrolRadius = 500.0f;

    /** Maximum distance for player detection and engagement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "200.0", ClampMax = "3000.0"))
    float DetectionRange = 1200.0f;

    /** AI reaction time before initiating response behaviors */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float ReactionTime = 0.5f;

    /** Enable ACF Ultimate combat behavior integration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    bool bUseACFCombatBehavior = true;

    /** Preferred combat engagement type for ACF combat system */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    ECombatBehaviorType PreferredCombatType = ECombatBehaviorType::EMelee;

    /** Movement speed multiplier during different AI states */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float MovementSpeed = 1.0f;

    /** Enable advanced pathfinding for complex navigation scenarios */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    bool bUseAdvancedPathfinding = false;

    /** Player detection range for stealth detection component */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
    float PlayerDetectionRange = 1000.0f;

    /** Maximum chase distance before returning to patrol */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float MaxChaseDistance = 1500.0f;

    FPortalAIConfig()
    {
        PatrolRadius = 500.0f;
        DetectionRange = 1200.0f;
        ReactionTime = 0.5f;
        bUseACFCombatBehavior = true;
        PreferredCombatType = ECombatBehaviorType::EMelee;
        MovementSpeed = 1.0f;
        bUseAdvancedPathfinding = false;
        PlayerDetectionRange = 1000.0f;
        MaxChaseDistance = 1500.0f;
    }
};

/**
 * Portal Defense AI Data Structure
 * Runtime data tracking for AI performance and state management
 * Integrated with AI LOD Manager for performance optimization
 */
USTRUCT(BlueprintType)
struct PORTAL_API FPortalAIData {
    GENERATED_BODY()

    /** Current AI configuration settings */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    FPortalAIConfig Config;

    /** Current movement speed (can be modified by upgrades) */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float MovementSpeed = 1.0f;

    /** Enhanced player detection range (modified by intelligence upgrades) */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float PlayerDetectionRange = 1000.0f;

    /** Enable advanced tactical behaviors */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    bool bUseAdvancedPathfinding = false;

    /** Combat accuracy modifier for ACF targeting systems */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float CombatAccuracy = 1.0f;

    /** Response time optimization factor */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float ResponseTimeMultiplier = 1.0f;

    FPortalAIData()
    {
        Config = FPortalAIConfig();
        MovementSpeed = 1.0f;
        PlayerDetectionRange = 1000.0f;
        bUseAdvancedPathfinding = false;
        CombatAccuracy = 1.0f;
        ResponseTimeMultiplier = 1.0f;
    }
};

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
 */
UCLASS(BlueprintType, meta = (DisplayName = "Portal Defense AI Controller"))
class PORTAL_API APortalDefenseAIController : public AACFAIController {
    GENERATED_BODY()

public:
    APortalDefenseAIController();

protected:
    // Core Unreal Engine Lifecycle
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Core Portal Defense Functions
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPortalTarget(APortalCore* NewTarget);

    /**
     * Start patrolling around a specific location with defined radius
     * @param Center The center point for patrol behavior
     * @param Radius The patrol radius around the center point
     */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StartPatrolling(FVector Center, float Radius = 500.0f);

    /** Start patrolling using current patrol center and configured radius */
    UFUNCTION(BlueprintCallable, Category = "Portal Defense", DisplayName = "Start Patrolling at Current Center")
    void BeginPatrolling() { StartPatrolling(PatrolCenter, AIConfig.PatrolRadius); }

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void InvestigateLocation(FVector Location);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void OnPlayerDetected(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void OnPlayerLost();

    // LOD System Integration for Performance Optimization
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

    // Configuration and Legacy Support Functions
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPatrolCenter(FVector Center) { StartPatrolling(Center, AIConfig.PatrolRadius); }

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPatrolRadius(float Radius) { AIConfig.PatrolRadius = Radius; }

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StopPatrolling() { GetWorldTimerManager().ClearTimer(PatrolTimer); }

    // Elite AI Integration with Intelligence Component
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void ActivateEliteMode();

    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void DeactivateEliteMode();

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    bool IsEliteModeActive() const { return bEnableEliteMode; }

    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetEliteActivationDistance(float Distance) { EliteActivationDistance = Distance; }

    // Data Access and Configuration
    UFUNCTION(BlueprintPure, Category = "AI Data")
    FPortalAIData GetCurrentAIData() const { return CurrentAIData; }

    UFUNCTION(BlueprintCallable, Category = "AI Data")
    void SetAIData(const FPortalAIData& NewData) { CurrentAIData = NewData; }

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    EPortalAIState GetCurrentState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    APortalCore* GetPortalTarget() const { return PortalTarget; }

    // Component Access for Advanced Integration
    UFUNCTION(BlueprintPure, Category = "Components")
    UACFStealthDetectionComponent* GetStealthDetectionComponent() const { return StealthComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    UEliteAIIntelligenceComponent* GetEliteIntelligenceComponent() const { return EliteIntelligence; }

protected:
    // Core Portal Defense Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Defense", meta = (DisplayName = "AI Configuration"))
    FPortalAIConfig AIConfig;

    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    TObjectPtr<APortalCore> PortalTarget;

    UPROPERTY(BlueprintReadOnly, Category = "AI State")
    EPortalAIState CurrentState = EPortalAIState::Patrolling;

    // AI Performance and Intelligence Data
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    FPortalAIData CurrentAIData;

    // ACF Ultimate Component Integration
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ACF Components", meta = (DisplayName = "Stealth Detection"))
    TObjectPtr<UACFStealthDetectionComponent> StealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ACF Components", meta = (DisplayName = "Elite Intelligence"))
    TObjectPtr<UEliteAIIntelligenceComponent> EliteIntelligence;

    // Elite AI System Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI", meta = (DisplayName = "Enable Elite Mode"))
    bool bEnableEliteMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI", meta = (ClampMin = "500.0", ClampMax = "2000.0"))
    float EliteActivationDistance = 1000.0f;

    // Patrol System Properties
    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    FVector PatrolCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    TArray<FVector> PatrolPoints;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    int32 CurrentPatrolIndex = 0;

    // Player Tracking and Detection
    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    TObjectPtr<APawn> DetectedPlayer;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Detection")
    float LastPlayerDetectionTime = 0.0f;

    // Performance and LOD Integration
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    TObjectPtr<UAILODManager> LODManager;

    // Timer Management for Performance Optimization
    FTimerHandle PatrolTimer;
    FTimerHandle InvestigationTimer;
    FTimerHandle EliteUpdateTimer;

private:
    // Internal Initialization and Setup
    void InitializeComponents();
    void RegisterWithManagers();
    void SetupACFIntegration();

    // Patrol System Implementation
    void GeneratePatrolPoints();
    void MoveToNextPatrolPoint();
    void OnPatrolPointReached();

    // Elite AI System Management
    void UpdateEliteSystemsActivation();
    void ConfigureEliteCapabilities();

    // Performance Optimization Helpers
    void OptimizeForCurrentLOD();
    void UpdatePerformanceMetrics();

    // ACF Combat System Integration
    void ConfigureCombatBehavior();
    void UpdateCombatState();
    bool ShouldEngageInCombat() const;

    // Internal State Management
    void TransitionToState(EPortalAIState NewState);
    void HandleStateTransition(EPortalAIState FromState, EPortalAIState ToState);
};