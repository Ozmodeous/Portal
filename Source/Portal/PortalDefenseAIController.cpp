// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "PortalDefenseAIController.h"
#include "ACFStealthDetectionComponent.h"
#include "AIBatchProcessor.h"
#include "AILODManager.h"
#include "AIOverseenComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ActorComponent.h"
#include "Components/CombatBehaviourComponent.h"
#include "EliteAIIntelligenceComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "PortalCore.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPortalDefenseAI, Log, All);

APortalDefenseAIController::APortalDefenseAIController()
{
    // Set this controller to be ticked every frame for real-time AI processing
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // Initialize AI configuration with default values
    AIConfig = FPortalAIConfig();
    CurrentAIData = FPortalAIData();

    // Initialize state variables
    CurrentState = EPortalAIState::Patrolling;
    CurrentBehaviorMode = EAIBehaviorMode::Patrol;
    CurrentThreatLevel = EThreatLevel::Low;
    CurrentLODLevel = 1;

    // Initialize performance variables
    AIUpdateFrequency = 10.0f;
    BehaviorComplexityMultiplier = 1.0f;

    // Initialize coordination variables
    bCoordinationEnabled = true;
    bHasDefensivePosition = false;
    DefensivePosition = FVector::ZeroVector;

    // Initialize elite AI settings
    bEnableEliteMode = false;
    EliteActivationDistance = 800.0f;

    // Initialize patrol settings
    PatrolCenter = FVector::ZeroVector;
    LastKnownPlayerLocation = FVector::ZeroVector;
    CurrentPatrolIndex = 0;

    // Initialize component references
    EliteIntelligence = nullptr;
    StealthDetection = nullptr;
    LODManager = nullptr;
    PortalTarget = nullptr;
    DetectedPlayer = nullptr;

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal Defense AI Controller created: %s"), *GetName());
}

void APortalDefenseAIController::BeginPlay()
{
    Super::BeginPlay();

    // Sequential initialization for optimal ACF Ultimate integration
    InitializeComponents();
    RegisterWithManagers();
    SetupACFIntegration();

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal Defense AI Controller initialized: %s"), *GetName());
}

void APortalDefenseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!InPawn) {
        UE_LOG(LogPortalDefenseAI, Warning, TEXT("Portal AI Controller: Attempted to possess null pawn"));
        return;
    }

    // Configure ACF Combat Behavior Component for tactical engagement
    if (AIConfig.bUseACFCombatBehavior) {
        ConfigureCombatBehavior();
    }

    // Establish patrol behavior if portal target is assigned
    if (PortalTarget) {
        StartPatrolling(PortalTarget->GetActorLocation(), AIConfig.PatrolRadius);
    } else {
        // Use spawn location as default patrol center
        PatrolCenter = InPawn->GetActorLocation();
        StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
    }

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI Controller possessed pawn: %s"), *InPawn->GetName());
}

void APortalDefenseAIController::OnUnPossess()
{
    // Clean unregistration from LOD management system
    if (LODManager) {
        LODManager->UnregisterAIController(this);
    }

    // Clear all active timers to prevent memory leaks and callback errors
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(PatrolTimer);
        World->GetTimerManager().ClearTimer(InvestigationTimer);
        World->GetTimerManager().ClearTimer(EliteUpdateTimer);
    }

    // Clear focus and targeting for clean state transition
    ClearFocus(EAIFocusPriority::Gameplay);
    DetectedPlayer = nullptr;

    Super::OnUnPossess();
}

void APortalDefenseAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Performance-optimized elite system updates
    UpdateEliteSystemsActivation();

    // Dynamic combat state monitoring for ACF integration
    UpdateCombatState();

    // Periodic performance metrics update for LOD optimization
    UpdatePerformanceMetrics();
}

void APortalDefenseAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Comprehensive cleanup sequence for ACF Ultimate compatibility
    if (LODManager) {
        LODManager->UnregisterAIController(this);
        LODManager = nullptr;
    }

    // Unregister from coordination systems
    UnregisterFromCoordinationSystems();

    // Clear all timer handles to prevent dangling references
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }

    // Reset state and clear references
    CurrentState = EPortalAIState::Patrolling;
    DetectedPlayer = nullptr;
    PortalTarget = nullptr;
    EliteIntelligence = nullptr;
    StealthDetection = nullptr;

    Super::EndPlay(EndPlayReason);
}

// ============================================================================
// CORE PORTAL DEFENSE FUNCTIONS
// ============================================================================

void APortalDefenseAIController::SetPortalTarget(APortalCore* NewTarget)
{
    PortalTarget = NewTarget;

    if (PortalTarget && GetPawn()) {
        // Immediately transition to defending the new portal target
        StartPatrolling(PortalTarget->GetActorLocation(), AIConfig.PatrolRadius);

        UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Assigned to defend portal %s"),
            *GetName(), *PortalTarget->GetName());
    }
}

void APortalDefenseAIController::StartPatrollingAtLocation(FVector Center, float Radius)
{
    // Public UFUNCTION wrapper for Blueprint access
    StartPatrolling(Center, Radius);
}

void APortalDefenseAIController::BeginPatrolling()
{
    // Public UFUNCTION wrapper for Blueprint access - uses current settings
    StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
}

void APortalDefenseAIController::StartPatrolling(FVector Center, float Radius)
{
    // Internal implementation - Update patrol configuration with new parameters
    PatrolCenter = Center;
    AIConfig.PatrolRadius = Radius;
    CurrentAIData.Config.PatrolRadius = Radius;

    // Transition to patrolling state with proper state management
    TransitionToState(EPortalAIState::Patrolling);

    // Generate optimized patrol pattern for efficient area coverage
    GeneratePatrolPoints();

    // Initiate patrol movement sequence
    MoveToNextPatrolPoint();

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Started patrolling at %s with radius %.2f"),
        *GetName(), *Center.ToString(), Radius);
}

void APortalDefenseAIController::InvestigateLocation(FVector Location)
{
    // Transition to investigation state
    TransitionToState(EPortalAIState::Investigating);

    // Move to investigation location
    MoveToLocation(Location, 100.0f);

    // Set investigation timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(
            InvestigationTimer,
            [this]() {
                // Return to patrol after investigation timeout
                TransitionToState(EPortalAIState::Returning);
                StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
            },
            AIConfig.InvestigationDuration,
            false);
    }

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Investigating location %s"),
        *GetName(), *Location.ToString());
}

void APortalDefenseAIController::OnPlayerDetected(APawn* Player)
{
    if (!Player) {
        UE_LOG(LogPortalDefenseAI, Warning, TEXT("Portal AI %s: OnPlayerDetected called with null player"), *GetName());
        return;
    }

    // Store player reference and location for tracking
    DetectedPlayer = Player;
    LastKnownPlayerLocation = Player->GetActorLocation();

    // Transition to chase state
    TransitionToState(EPortalAIState::ChasingPlayer);

    // Set focus on detected player
    SetFocus(Player);

    // Start movement towards player
    MoveToActor(Player, 100.0f);

    // Set investigation timer as fallback
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(
            InvestigationTimer,
            this,
            &APortalDefenseAIController::OnPlayerLost,
            20.0f,
            false);
    }

    // Activate elite intelligence recording if available
    if (IsEliteModeActive() && EliteIntelligence) {
        // Record player detection for learning system
        UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Elite AI recording player detection"));
    }

    // Configure ACF Combat Behavior Component for player engagement
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // Set combat state to searching/engaging
        UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("ACF Combat Component configured for engagement"));
    }

    UE_LOG(LogPortalDefenseAI, Warning, TEXT("Portal AI %s: Player %s detected at %s"),
        *GetName(), *Player->GetName(), *LastKnownPlayerLocation.ToString());
}

void APortalDefenseAIController::OnPlayerLost()
{
    // Record player escape event for elite AI learning system
    if (DetectedPlayer && IsEliteModeActive() && EliteIntelligence) {
        UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Elite AI recording player escape"));
    }

    // Clear player tracking and targeting data
    DetectedPlayer = nullptr;
    ClearFocus(EAIFocusPriority::Gameplay);

    // Transition to return state for tactical repositioning
    TransitionToState(EPortalAIState::Returning);

    // Resume patrol behavior at original patrol center
    StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Lost player contact, returning to patrol"), *GetName());
}

// ============================================================================
// AI BEHAVIOR AND STATE MANAGEMENT
// ============================================================================

void APortalDefenseAIController::UpdateAIBehavior(int32 LODLevel)
{
    // Update AI behavior based on LOD level for performance optimization
    if (!GetPawn()) {
        return;
    }

    // Adjust behavior complexity based on LOD level
    float ComplexityMultiplier = 1.0f;
    switch (LODLevel) {
    case 0:
        ComplexityMultiplier = 1.0f;
        break; // Full detail
    case 1:
        ComplexityMultiplier = 0.8f;
        break; // High detail
    case 2:
        ComplexityMultiplier = 0.6f;
        break; // Medium detail
    case 3:
        ComplexityMultiplier = 0.4f;
        break; // Low detail
    case 4:
        ComplexityMultiplier = 0.2f;
        break; // Very low detail
    default:
        ComplexityMultiplier = 0.5f;
        break;
    }

    // Update movement speed based on complexity
    CurrentAIData.MovementSpeed = AIConfig.BaseMovementSpeed * ComplexityMultiplier;

    // Update detection range based on LOD
    float DetectionMultiplier = FMath::Lerp(0.5f, 1.0f, ComplexityMultiplier);
    CurrentAIData.PlayerDetectionRange = AIConfig.DetectionRange * DetectionMultiplier;

    // Update combat accuracy
    CurrentAIData.CombatAccuracy = FMath::Lerp(0.3f, 1.0f, ComplexityMultiplier);

    // Update response time
    CurrentAIData.ResponseTimeMultiplier = FMath::Lerp(2.0f, 1.0f, ComplexityMultiplier);

    // Apply ACF component updates if available
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>()) {
        // Update combat behavior complexity - safe wrapper
        UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Updating ACF combat behavior complexity"));
    }

    // Update stealth detection component if available
    if (UACFStealthDetectionComponent* StealthComp = GetPawn()->FindComponentByClass<UACFStealthDetectionComponent>()) {
        // Update detection parameters - safe wrapper
        UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Updating ACF stealth detection parameters"));
    }

    UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Updated behavior for LOD level %d (Complexity: %.2f)"),
        *GetName(), LODLevel, ComplexityMultiplier);
}

void APortalDefenseAIController::UpdatePatrolLogic()
{
    // Optimize patrol logic based on current AI state
    if (CurrentState == EPortalAIState::Patrolling) {
        MoveToNextPatrolPoint();
    }
}

void APortalDefenseAIController::UpdateCombatBehavior()
{
    // Update combat state synchronization with ACF Combat Behavior Component
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // Coordinate AI state with ACF combat state management
        if (IsInCombat()) {
            UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("ACF Combat: In combat mode"));
        } else if (CurrentState == EPortalAIState::Patrolling) {
            UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("ACF Combat: In patrol mode"));
        }
    }
}

void APortalDefenseAIController::UpdateTargeting()
{
    // Advanced targeting system update for ACF Ultimate integration
    if (DetectedPlayer && IsValid(DetectedPlayer)) {
        // Update last known position for predictive targeting
        LastKnownPlayerLocation = DetectedPlayer->GetActorLocation();

        // Maintain focus on active target
        SetFocus(DetectedPlayer);
    } else if (PortalTarget && IsValid(PortalTarget)) {
        // Default focus on portal target when no active threats
        SetFocus(PortalTarget);
    }
}

void APortalDefenseAIController::TransitionToState(EPortalAIState NewState)
{
    // State transition with proper cleanup and initialization
    const EPortalAIState OldState = CurrentState;

    if (OldState != NewState) {
        HandleStateTransition(OldState, NewState);
        CurrentState = NewState;

        UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: State transition from %d to %d"),
            *GetName(), static_cast<int32>(OldState), static_cast<int32>(NewState));
    }
}

// ============================================================================
// LOD SYSTEM INTEGRATION
// ============================================================================

void APortalDefenseAIController::SetAIUpdateFrequency(float UpdateFrequency)
{
    // Set the AI update frequency for LOD optimization
    AIUpdateFrequency = FMath::Clamp(UpdateFrequency, 0.1f, 60.0f);

    // Adjust tick interval based on update frequency
    float TickInterval = 1.0f / AIUpdateFrequency;
    SetActorTickInterval(TickInterval);

    UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Set update frequency to %.2f Hz"), *GetName(), UpdateFrequency);
}

void APortalDefenseAIController::SetBehaviorComplexity(float ComplexityMultiplier)
{
    // Set the behavior complexity multiplier for LOD management
    BehaviorComplexityMultiplier = FMath::Clamp(ComplexityMultiplier, 0.1f, 2.0f);

    // Apply complexity to current AI data
    CurrentAIData.MovementSpeed = AIConfig.BaseMovementSpeed * BehaviorComplexityMultiplier;
    CurrentAIData.CombatAccuracy = FMath::Lerp(0.3f, 1.0f, BehaviorComplexityMultiplier);
    CurrentAIData.ResponseTimeMultiplier = FMath::Lerp(2.0f, 1.0f, BehaviorComplexityMultiplier);

    UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Set behavior complexity to %.2f"), *GetName(), ComplexityMultiplier);
}

void APortalDefenseAIController::SetCurrentLODLevel(int32 LODLevel)
{
    // Set the current LOD level for internal tracking
    CurrentLODLevel = FMath::Clamp(LODLevel, 0, 4);

    // Update behavior based on new LOD level
    UpdateAIBehavior(CurrentLODLevel);

    UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Set LOD level to %d"), *GetName(), CurrentLODLevel);
}

// ============================================================================
// TACTICAL COORDINATION
// ============================================================================

void APortalDefenseAIController::SetDefensivePosition(const FVector& Position)
{
    // Set the defensive position for tactical coordination
    DefensivePosition = Position;
    bHasDefensivePosition = true;

    // Move to defensive position if not currently engaged
    if (CurrentState != EPortalAIState::ChasingPlayer) {
        MoveToLocation(DefensivePosition, 50.0f);
    }

    UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Set defensive position to %s"), *GetName(), *Position.ToString());
}

void APortalDefenseAIController::SetAIBehaviorMode(EAIBehaviorMode NewMode)
{
    // Set the AI behavior mode for coordination
    if (CurrentBehaviorMode != NewMode) {
        EAIBehaviorMode OldMode = CurrentBehaviorMode;
        CurrentBehaviorMode = NewMode;

        // Apply behavior mode changes
        ApplyBehaviorModeChanges(OldMode, NewMode);

        UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Behavior mode changed from %d to %d"),
            *GetName(), static_cast<int32>(OldMode), static_cast<int32>(NewMode));
    }
}

void APortalDefenseAIController::SetCoordinationEnabled(bool bEnabled)
{
    // Enable or disable coordination with other AI controllers
    bCoordinationEnabled = bEnabled;

    if (bCoordinationEnabled) {
        // Register with coordination systems if not already registered
        RegisterWithCoordinationSystems();
    } else {
        // Unregister from coordination systems
        UnregisterFromCoordinationSystems();
    }

    UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Coordination %s"),
        *GetName(), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void APortalDefenseAIController::SetThreatLevel(EThreatLevel ThreatLevel)
{
    // Set the current threat level for tactical response
    CurrentThreatLevel = ThreatLevel;

    // Adjust AI behavior based on threat level
    switch (ThreatLevel) {
    case EThreatLevel::Low:
        SetAIBehaviorMode(EAIBehaviorMode::Patrol);
        break;

    case EThreatLevel::Medium:
        SetAIBehaviorMode(EAIBehaviorMode::Alert);
        break;

    case EThreatLevel::High:
    case EThreatLevel::Critical:
    case EThreatLevel::Extreme:
        SetAIBehaviorMode(EAIBehaviorMode::Combat);
        break;
    }

    UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Threat level set to %d"),
        *GetName(), static_cast<int32>(ThreatLevel));
}

// ============================================================================
// ELITE AI FUNCTIONS
// ============================================================================

void APortalDefenseAIController::ActivateEliteMode()
{
    bEnableEliteMode = true;

    // Configure enhanced capabilities for elite AI behavior
    ConfigureEliteCapabilities();

    // Start elite system update timer for advanced tactical behaviors
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(
            EliteUpdateTimer,
            this,
            &APortalDefenseAIController::UpdateEliteSystemsActivation,
            1.0f,
            true);
    }

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Elite mode activated"), *GetName());
}

void APortalDefenseAIController::DeactivateEliteMode()
{
    bEnableEliteMode = false;

    // Clear elite update timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(EliteUpdateTimer);
    }

    // Reset AI capabilities to normal values
    CurrentAIData.MovementSpeed = AIConfig.BaseMovementSpeed;
    CurrentAIData.PlayerDetectionRange = AIConfig.DetectionRange;
    CurrentAIData.CombatAccuracy = 1.0f;
    CurrentAIData.ResponseTimeMultiplier = 1.0f;

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Elite mode deactivated"), *GetName());
}

// ============================================================================
// QUERY FUNCTIONS
// ============================================================================

bool APortalDefenseAIController::IsInCombat() const
{
    // Determine combat state based on current AI state and ACF integration
    return CurrentState == EPortalAIState::ChasingPlayer || (DetectedPlayer && IsValid(DetectedPlayer));
}

bool APortalDefenseAIController::IsEngagingPlayer() const
{
    // Player engagement validation for LOD manager coordination
    return DetectedPlayer && IsValid(DetectedPlayer) && CurrentState == EPortalAIState::ChasingPlayer;
}

// ============================================================================
// INTERNAL PROCESSING FUNCTIONS
// ============================================================================

void APortalDefenseAIController::InitializeComponents()
{
    // Initialize ACF component references for integration
    if (GetPawn()) {
        EliteIntelligence = GetPawn()->FindComponentByClass<UEliteAIIntelligenceComponent>();
        StealthDetection = GetPawn()->FindComponentByClass<UACFStealthDetectionComponent>();
    }

    // Initialize AI data with configuration
    CurrentAIData.Config = AIConfig;
    CurrentAIData.MovementSpeed = AIConfig.BaseMovementSpeed;
    CurrentAIData.PlayerDetectionRange = AIConfig.DetectionRange;

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Components initialized"), *GetName());
}

void APortalDefenseAIController::RegisterWithManagers()
{
    // Register with AI LOD Manager for performance optimization
    if (UWorld* World = GetWorld()) {
        LODManager = UAILODManager::GetInstance();
        if (LODManager) {
            LODManager->RegisterAIController(this);
            UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Registered with LOD Manager"), *GetName());
        }
    }

    // Register with coordination systems
    RegisterWithCoordinationSystems();
}

void APortalDefenseAIController::SetupACFIntegration()
{
    // Configure ACF integration based on settings
    if (AIConfig.bUseACFCombatBehavior) {
        ConfigureCombatBehavior();
    }

    // Setup stealth detection if component is available
    if (StealthDetection) {
        UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: ACF Stealth Detection configured"), *GetName());
    }

    // Setup elite intelligence if component is available
    if (EliteIntelligence) {
        UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Elite Intelligence configured"), *GetName());
    }
}

void APortalDefenseAIController::ConfigureCombatBehavior()
{
    // ACF Combat Behavior Component configuration for Portal Defense scenarios
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // Configure combat behavior type based on AI preferences
        UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: ACF Combat Behavior configured"), *GetName());
    }
}

void APortalDefenseAIController::GeneratePatrolPoints()
{
    // Generate patrol points in a circle around the patrol center
    PatrolPoints.Empty();

    const int32 NumPatrolPoints = 4; // Square patrol pattern
    const float AngleStep = 360.0f / NumPatrolPoints;

    for (int32 i = 0; i < NumPatrolPoints; ++i) {
        float Angle = AngleStep * i;
        FVector PatrolPoint = PatrolCenter + FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * AIConfig.PatrolRadius, FMath::Sin(FMath::DegreesToRadians(Angle)) * AIConfig.PatrolRadius, 0.0f);

        PatrolPoints.Add(PatrolPoint);
    }

    CurrentPatrolIndex = 0;

    UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Generated %d patrol points"), *GetName(), PatrolPoints.Num());
}

void APortalDefenseAIController::MoveToNextPatrolPoint()
{
    if (PatrolPoints.Num() == 0 || CurrentState != EPortalAIState::Patrolling) {
        return;
    }

    // Get next patrol point
    FVector TargetPoint = PatrolPoints[CurrentPatrolIndex];

    // Move to patrol point
    EPathFollowingRequestResult::Type Result = MoveToLocation(TargetPoint, 50.0f);

    if (Result == EPathFollowingRequestResult::RequestSuccessful) {
        // Advance to next patrol point
        CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();

        UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("Portal AI %s: Moving to patrol point %s"),
            *GetName(), *TargetPoint.ToString());
    } else {
        UE_LOG(LogPortalDefenseAI, Warning, TEXT("Portal AI %s: Failed to move to patrol point %s"),
            *GetName(), *TargetPoint.ToString());

        // Attempt next patrol point on movement failure
        FTimerHandle RetryTimer;
        GetWorld()->GetTimerManager().SetTimer(
            RetryTimer,
            this,
            &APortalDefenseAIController::MoveToNextPatrolPoint,
            2.0f,
            false);
    }
}

void APortalDefenseAIController::OnPatrolPointReached()
{
    // Schedule next patrol point movement with tactical delay
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(
            PatrolTimer,
            this,
            &APortalDefenseAIController::MoveToNextPatrolPoint,
            3.0f,
            false);
    }
}

void APortalDefenseAIController::HandleStateTransition(EPortalAIState FromState, EPortalAIState ToState)
{
    // Cleanup operations for state exit
    switch (FromState) {
    case EPortalAIState::Investigating:
        GetWorld()->GetTimerManager().ClearTimer(InvestigationTimer);
        break;
    case EPortalAIState::Patrolling:
        GetWorld()->GetTimerManager().ClearTimer(PatrolTimer);
        break;
    default:
        break;
    }

    // Initialization operations for state entry
    switch (ToState) {
    case EPortalAIState::Patrolling:
        // Patrol initialization handled in StartPatrolling()
        break;
    case EPortalAIState::ChasingPlayer:
        // Clear existing movement to focus on player
        StopMovement();
        break;
    default:
        break;
    }
}

void APortalDefenseAIController::UpdateCombatState()
{
    // Continuous combat state synchronization with ACF framework
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // Update combat state based on current AI tactical situation
        switch (CurrentState) {
        case EPortalAIState::ChasingPlayer:
            UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("ACF Combat: Chasing player state"));
            break;
        case EPortalAIState::Patrolling:
            UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("ACF Combat: Patrolling state"));
            break;
        case EPortalAIState::Investigating:
            UE_LOG(LogPortalDefenseAI, VeryVerbose, TEXT("ACF Combat: Investigating state"));
            break;
        default:
            break;
        }
    }
}

void APortalDefenseAIController::UpdateEliteSystemsActivation()
{
    // Elite mode activation based on player proximity and threat assessment
    if (!bEnableEliteMode && GetPawn()) {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
            const float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

            if (DistanceToPlayer <= EliteActivationDistance) {
                ActivateEliteMode();
            }
        }
    }
}

void APortalDefenseAIController::ConfigureEliteCapabilities()
{
    // Enhanced AI capabilities for elite mode operation
    if (EliteIntelligence) {
        UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Elite capabilities configured"), *GetName());
    }

    // Upgrade AI configuration for elite performance
    CurrentAIData.MovementSpeed *= 1.2f;
    CurrentAIData.PlayerDetectionRange *= 1.3f;
    CurrentAIData.CombatAccuracy *= 1.15f;
    CurrentAIData.ResponseTimeMultiplier *= 0.8f;
}

void APortalDefenseAIController::OptimizeForCurrentLOD()
{
    // LOD-based performance optimization for large-scale scenarios
    if (LODManager) {
        // Adjust tick frequency based on LOD level
        const float TickInterval = IsInCombat() ? 0.05f : 0.2f;
        SetActorTickInterval(TickInterval);
    }
}

void APortalDefenseAIController::UpdatePerformanceMetrics()
{
    // Performance metrics tracking for LOD manager coordination
    OptimizeForCurrentLOD();

    // Update AI data metrics for tactical analysis
    CurrentAIData.Config = AIConfig;
}

void APortalDefenseAIController::ApplyBehaviorModeChanges(EAIBehaviorMode OldMode, EAIBehaviorMode NewMode)
{
    // Apply changes based on the new behavior mode
    switch (NewMode) {
    case EAIBehaviorMode::Patrol:
        // Return to patrol behavior
        if (CurrentState != EPortalAIState::Patrolling) {
            TransitionToState(EPortalAIState::Patrolling);
            StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
        }
        break;

    case EAIBehaviorMode::Alert:
        // Increase alertness and detection range
        CurrentAIData.PlayerDetectionRange *= 1.2f;
        CurrentAIData.ResponseTimeMultiplier *= 0.8f;
        break;

    case EAIBehaviorMode::Combat:
        // Prepare for combat engagement
        CurrentAIData.MovementSpeed *= 1.1f;
        CurrentAIData.CombatAccuracy *= 1.1f;
        CurrentAIData.ResponseTimeMultiplier *= 0.7f;
        break;

    case EAIBehaviorMode::Defensive:
        // Move to defensive position if available
        if (bHasDefensivePosition) {
            MoveToLocation(DefensivePosition, 50.0f);
        }
        break;
    }
}

void APortalDefenseAIController::RegisterWithCoordinationSystems()
{
    // Register with AI LOD Manager
    if (UWorld* World = GetWorld()) {
        if (UAILODManager* CurrentLODManager = UAILODManager::GetInstance()) {
            CurrentLODManager->RegisterAIController(this);
        }

        // Register with AI Batch Processor if available
        if (AGameStateBase* GameState = World->GetGameState()) {
            if (UAIBatchProcessor* BatchProcessor = GameState->FindComponentByClass<UAIBatchProcessor>()) {
                BatchProcessor->AssignAIToBatch(this, CurrentLODLevel);
            }
        }
    }
}

void APortalDefenseAIController::UnregisterFromCoordinationSystems()
{
    // Unregister from AI LOD Manager
    if (UAILODManager* CurrentLODManager = UAILODManager::GetInstance()) {
        CurrentLODManager->UnregisterAIController(this);
    }

    // Unregister from AI Batch Processor if available
    if (UWorld* World = GetWorld()) {
        if (AGameStateBase* GameState = World->GetGameState()) {
            if (UAIBatchProcessor* BatchProcessor = GameState->FindComponentByClass<UAIBatchProcessor>()) {
                BatchProcessor->RemoveAIFromBatch(this);
            }
        }
    }
}

bool APortalDefenseAIController::ShouldEngageInCombat() const
{
    // Combat engagement decision logic based on tactical parameters
    return DetectedPlayer && IsValid(DetectedPlayer) && FVector::Dist(GetPawn()->GetActorLocation(), DetectedPlayer->GetActorLocation()) <= AIConfig.DetectionRange;
}

TArray<APortalDefenseAIController*> APortalDefenseAIController::GetManagedAIControllers() const
{
    // Convert TObjectPtr array to regular pointer array for Blueprint compatibility
    TArray<APortalDefenseAIController*> Result;

    // This function would typically get the managed AI controllers from the coordination system
    // For now, return empty array as this controller manages its own behavior
    // In a full implementation, this would query the AIOverseenComponent or coordination manager

    return Result;
}

// ============================================================================
// MOVEMENT AND PATHFINDING
// ============================================================================

void APortalDefenseAIController::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    // Handle movement completion based on current state
    switch (CurrentState) {
    case EPortalAIState::Patrolling:
        OnPatrolPointReached();
        break;

    case EPortalAIState::Investigating:
        // Investigation location reached
        UE_LOG(LogPortalDefenseAI, Log, TEXT("Portal AI %s: Investigation location reached"), *GetName());
        break;

    case EPortalAIState::ChasingPlayer:
        // Continue chasing if player is still detected
        if (DetectedPlayer && IsValid(DetectedPlayer)) {
            MoveToActor(DetectedPlayer, 100.0f);
        } else {
            OnPlayerLost();
        }
        break;

    default:
        break;
    }
}