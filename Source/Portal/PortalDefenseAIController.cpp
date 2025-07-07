// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "PortalDefenseAIController.h"
#include "ACFStealthDetectionComponent.h"
#include "AILODManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ACFCombatBehaviourComponent.h"
#include "DrawDebugHelpers.h"
#include "EliteAIIntelligenceComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "PortalCore.h"

APortalDefenseAIController::APortalDefenseAIController()
{
    // Configure component tick settings for UE 5.5.4 performance optimization
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.TickInterval = 0.1f; // Optimized tick rate for AI responsiveness

    // Initialize ACF Ultimate component integration
    StealthComponent = CreateDefaultSubobject<UACFStealthDetectionComponent>(TEXT("StealthComponent"));
    EliteIntelligence = CreateDefaultSubobject<UEliteAIIntelligenceComponent>(TEXT("EliteIntelligence"));

    // Configure default AI state and behavior parameters
    CurrentState = EPortalAIState::Patrolling;
    bEnableEliteMode = false;
    EliteActivationDistance = 1000.0f;
    CurrentPatrolIndex = 0;

    // Initialize AI configuration with balanced defaults for Portal Defense scenarios
    AIConfig = FPortalAIConfig();
    CurrentAIData = FPortalAIData();

    // Null-initialize object references for safe operation
    PortalTarget = nullptr;
    DetectedPlayer = nullptr;
    LODManager = nullptr;

    // Initialize spatial tracking variables
    PatrolCenter = FVector::ZeroVector;
    LastKnownPlayerLocation = FVector::ZeroVector;
    LastPlayerDetectionTime = 0.0f;

    // Reserve patrol points array for performance optimization
    PatrolPoints.Reserve(8);
}

void APortalDefenseAIController::BeginPlay()
{
    Super::BeginPlay();

    // Sequential initialization for optimal ACF Ultimate integration
    InitializeComponents();
    RegisterWithManagers();
    SetupACFIntegration();

    UE_LOG(LogTemp, Log, TEXT("Portal Defense AI Controller initialized: %s"), *GetName());
}

void APortalDefenseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!InPawn) {
        UE_LOG(LogTemp, Warning, TEXT("Portal AI Controller: Attempted to possess null pawn"));
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

    UE_LOG(LogTemp, Verbose, TEXT("Portal AI Controller possessed pawn: %s"), *InPawn->GetName());
}

void APortalDefenseAIController::OnUnPossess()
{
    // Clean unregistration from LOD management system
    if (LODManager) {
        LODManager->UnregisterAI(this);
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
        LODManager->UnregisterAI(this);
        LODManager = nullptr;
    }

    // Clear all timer handles to prevent dangling references
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }

    // Reset state and clear references
    CurrentState = EPortalAIState::Patrolling;
    DetectedPlayer = nullptr;
    PortalTarget = nullptr;

    Super::EndPlay(EndPlayReason);
}

void APortalDefenseAIController::SetPortalTarget(APortalCore* NewTarget)
{
    PortalTarget = NewTarget;

    if (PortalTarget && GetPawn()) {
        // Immediately transition to defending the new portal target
        StartPatrolling(PortalTarget->GetActorLocation(), AIConfig.PatrolRadius);

        UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Assigned to defend portal %s"),
            *GetName(), *PortalTarget->GetName());
    }
}

void APortalDefenseAIController::StartPatrolling(FVector Center, float Radius)
{
    // Update patrol configuration with new parameters
    PatrolCenter = Center;
    AIConfig.PatrolRadius = Radius;
    CurrentAIData.Config.PatrolRadius = Radius;

    // Transition to patrolling state with proper state management
    TransitionToState(EPortalAIState::Patrolling);

    // Generate optimized patrol pattern for efficient area coverage
    GeneratePatrolPoints();

    // Initiate patrol movement sequence
    MoveToNextPatrolPoint();

    UE_LOG(LogTemp, Verbose, TEXT("Portal AI %s: Started patrolling at %s with radius %.1f"),
        *GetName(), *Center.ToString(), Radius);
}

void APortalDefenseAIController::InvestigateLocation(FVector Location)
{
    // Validate location before initiating investigation
    if (Location.IsZero()) {
        UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Invalid investigation location"), *GetName());
        return;
    }

    // Transition to investigation state with target location
    TransitionToState(EPortalAIState::Investigating);

    // Move to investigation target using ACF navigation system
    MoveToLocation(Location);

    // Schedule automatic return to patrol after investigation timeout
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(InvestigationTimer, [this]() {
            if (CurrentState == EPortalAIState::Investigating)
            {
                // Return to patrol after investigation completion
                StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
            } }, 8.0f, false);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Investigating location %s"),
        *GetName(), *Location.ToString());
}

void APortalDefenseAIController::OnPlayerDetected(APawn* Player)
{
    if (!Player || !IsValid(Player)) {
        UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Invalid player detection"), *GetName());
        return;
    }

    // Update player tracking data for tactical analysis
    DetectedPlayer = Player;
    LastKnownPlayerLocation = Player->GetActorLocation();
    LastPlayerDetectionTime = GetWorld()->GetTimeSeconds();

    // Transition to aggressive engagement state
    TransitionToState(EPortalAIState::ChasingPlayer);

    // Focus targeting system on detected player for ACF combat integration
    SetFocus(Player);

    // Request maximum LOD priority for enhanced combat performance
    if (LODManager) {
        LODManager->ForceMaximumLOD(this, 20.0f);
    }

    // Activate elite intelligence recording if available
    if (IsEliteModeActive() && EliteIntelligence) {
        EliteIntelligence->RecordPlayerAction(Player, LastKnownPlayerLocation, TEXT("Detected"));
    }

    // Configure ACF Combat Behavior Component for player engagement
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>()) {
        // ACF handles internal target assignment and threat escalation
        CombatComp->SetCurrentCombatState(EAICombatState::ESearching);
    }

    UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Player %s detected at %s"),
        *GetName(), *Player->GetName(), *LastKnownPlayerLocation.ToString());
}

void APortalDefenseAIController::OnPlayerLost()
{
    // Record player escape event for elite AI learning system
    if (DetectedPlayer && IsEliteModeActive() && EliteIntelligence) {
        EliteIntelligence->RecordPlayerAction(DetectedPlayer, LastKnownPlayerLocation, TEXT("Escaped"));
    }

    // Clear player tracking and targeting data
    DetectedPlayer = nullptr;
    ClearFocus(EAIFocusPriority::Gameplay);

    // Transition to return state for tactical repositioning
    TransitionToState(EPortalAIState::Returning);

    // Resume patrol behavior at original patrol center
    StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Lost player contact, returning to patrol"), *GetName());
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
            CombatComp->SetCurrentCombatState(EAICombatState::ESearching);
        } else if (CurrentState == EPortalAIState::Patrolling) {
            CombatComp->SetCurrentCombatState(EAICombatState::EWaiting);
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

void APortalDefenseAIController::ActivateEliteMode()
{
    bEnableEliteMode = true;

    // Configure enhanced capabilities for elite AI behavior
    ConfigureEliteCapabilities();

    // Start elite system update timer for advanced tactical behaviors
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(EliteUpdateTimer, this,
            &APortalDefenseAIController::UpdateEliteSystemsActivation, 1.0f, true);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Elite mode activated"), *GetName());
}

void APortalDefenseAIController::DeactivateEliteMode()
{
    bEnableEliteMode = false;

    // Clear elite update timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(EliteUpdateTimer);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Elite mode deactivated"), *GetName());
}

void APortalDefenseAIController::InitializeComponents()
{
    // Initialize ACF Stealth Detection Component with Portal-specific parameters
    if (StealthComponent) {
        StealthComponent->SetDetectionRange(AIConfig.DetectionRange);
        StealthComponent->SetReactionTime(AIConfig.ReactionTime);
    }

    // Configure Elite Intelligence Component for advanced tactical analysis
    if (EliteIntelligence) {
        EliteIntelligence->SetActivationDistance(EliteActivationDistance);
    }

    // Locate and cache LOD Manager reference for performance optimization
    if (UWorld* World = GetWorld()) {
        LODManager = UAILODManager::GetInstance(World);
    }
}

void APortalDefenseAIController::RegisterWithManagers()
{
    // Register with AI LOD Manager for performance optimization
    if (LODManager) {
        LODManager->RegisterAI(this);
        UE_LOG(LogTemp, Verbose, TEXT("Portal AI %s: Registered with LOD Manager"), *GetName());
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: LOD Manager not found"), *GetName());
    }
}

void APortalDefenseAIController::SetupACFIntegration()
{
    // Configure ACF framework integration for seamless combat system coordination
    if (GetPawn() && AIConfig.bUseACFCombatBehavior) {
        ConfigureCombatBehavior();
    }

    // Initialize blackboard values for ACF behavior tree integration
    if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent()) {
        // Set default patrol parameters in blackboard
        BlackboardComp->SetValueAsVector(TEXT("PatrolCenter"), PatrolCenter);
        BlackboardComp->SetValueAsFloat(TEXT("PatrolRadius"), AIConfig.PatrolRadius);
        BlackboardComp->SetValueAsFloat(TEXT("DetectionRange"), AIConfig.DetectionRange);
    }
}

void APortalDefenseAIController::GeneratePatrolPoints()
{
    // Clear existing patrol points for regeneration
    PatrolPoints.Empty();
    CurrentPatrolIndex = 0;

    // Generate optimized patrol pattern using mathematical distribution
    const int32 NumPoints = 6; // Hexagonal pattern for optimal coverage
    const float AngleStep = 360.0f / NumPoints;

    for (int32 i = 0; i < NumPoints; ++i) {
        const float Angle = FMath::DegreesToRadians(AngleStep * i);
        const FVector Offset = FVector(
            FMath::Cos(Angle) * AIConfig.PatrolRadius,
            FMath::Sin(Angle) * AIConfig.PatrolRadius,
            0.0f);

        const FVector PatrolPoint = PatrolCenter + Offset;
        PatrolPoints.Add(PatrolPoint);
    }

    UE_LOG(LogTemp, Verbose, TEXT("Portal AI %s: Generated %d patrol points"),
        *GetName(), PatrolPoints.Num());
}

void APortalDefenseAIController::MoveToNextPatrolPoint()
{
    // Validate patrol points array before movement
    if (PatrolPoints.Num() == 0) {
        GeneratePatrolPoints();
        return;
    }

    // Cycle through patrol points with wraparound
    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
    const FVector TargetPoint = PatrolPoints[CurrentPatrolIndex];

    // Execute movement using ACF navigation system
    const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(TargetPoint);

    if (MoveResult == EPathFollowingRequestResult::Failed) {
        UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Failed to move to patrol point %s"),
            *GetName(), *TargetPoint.ToString());

        // Attempt next patrol point on movement failure
        FTimerHandle RetryTimer;
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, this,
            &APortalDefenseAIController::MoveToNextPatrolPoint, 2.0f, false);
    }
}

void APortalDefenseAIController::OnPatrolPointReached()
{
    // Schedule next patrol point movement with tactical delay
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(PatrolTimer, this,
            &APortalDefenseAIController::MoveToNextPatrolPoint, 3.0f, false);
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
        EliteIntelligence->SetAdvancedTacticsEnabled(true);
        EliteIntelligence->SetPlayerTrackingEnabled(true);
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

void APortalDefenseAIController::ConfigureCombatBehavior()
{
    // ACF Combat Behavior Component configuration for Portal Defense scenarios
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>()) {
        CombatComp->DefaultCombatBehaviorType = AIConfig.PreferredCombatType;
        CombatComp->DefaultCombatState = (AIConfig.PreferredCombatType == ECombatBehaviorType::EMelee)
            ? EAICombatState::EMeleeCombat
            : EAICombatState::ERangedCombat;

        UE_LOG(LogTemp, Verbose, TEXT("Portal AI %s: Configured ACF combat behavior"), *GetName());
    }
}

void APortalDefenseAIController::UpdateCombatState()
{
    // Continuous combat state synchronization with ACF framework
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // Update combat state based on current AI tactical situation
        switch (CurrentState) {
        case EPortalAIState::ChasingPlayer:
            CombatComp->SetCurrentCombatState(EAICombatState::ESearching);
            break;
        case EPortalAIState::Patrolling:
            CombatComp->SetCurrentCombatState(EAICombatState::EWaiting);
            break;
        case EPortalAIState::Investigating:
            CombatComp->SetCurrentCombatState(EAICombatState::EInvestigating);
            break;
        default:
            break;
        }
    }
}

bool APortalDefenseAIController::ShouldEngageInCombat() const
{
    // Combat engagement decision logic based on tactical parameters
    return DetectedPlayer && IsValid(DetectedPlayer) && FVector::Dist(GetPawn()->GetActorLocation(), DetectedPlayer->GetActorLocation()) <= AIConfig.DetectionRange;
}

void APortalDefenseAIController::TransitionToState(EPortalAIState NewState)
{
    // State transition with proper cleanup and initialization
    const EPortalAIState OldState = CurrentState;

    if (OldState != NewState) {
        HandleStateTransition(OldState, NewState);
        CurrentState = NewState;

        UE_LOG(LogTemp, Verbose, TEXT("Portal AI %s: State transition from %d to %d"),
            *GetName(), static_cast<int32>(OldState), static_cast<int32>(NewState));
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