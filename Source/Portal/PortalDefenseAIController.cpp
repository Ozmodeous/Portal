// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "PortalDefenseAIController.h"
#include "AILODManager.h"
#include "AIOverlordManager.h"
#include "ARSStatisticsComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CollisionQueryParams.h"
#include "Components/ACFActionsManagerComponent.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Components/ACFCombatBehaviourComponent.h"
#include "Components/ACFDamageHandlerComponent.h"
#include "Components/ACFThreatManagerComponent.h"
#include "DrawDebugHelpers.h"
#include "EliteAIIntelligenceComponent.h" // Include here instead of in header
#include "Engine/World.h"
#include "Game/ACFFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Interfaces/ACFEntityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "PortalCore.h"

APortalDefenseAIController::APortalDefenseAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;

    // Create Core Components
    StealthComponent = CreateDefaultSubobject<UACFStealthDetectionComponent>(TEXT("StealthComponent"));
    EliteIntelligence = CreateDefaultSubobject<UEliteAIIntelligenceComponent>(TEXT("EliteIntelligence"));

    // Initialize AI Data
    BaseAIData.MovementSpeed = 400.0f;
    BaseAIData.PatrolRadius = 400.0f;
    BaseAIData.PlayerDetectionRange = 1200.0f;
    BaseAIData.AttackRange = 800.0f;
    BaseAIData.AccuracyMultiplier = 0.3f;
    BaseAIData.bUseAdvancedPathfinding = false;
    BaseAIData.bCanFlank = false;
    BaseAIData.AggressionLevel = 1.0f;
    BaseAIData.ReactionTime = 0.5f;
    BaseAIData.bUseACFActions = true;
    BaseAIData.PatrolSpeed = 0.5f;

    CurrentAIData = BaseAIData;

    // Elite AI Configuration
    bEnableEliteMode = false;
    EliteDifficulty = EEliteDifficultyLevel::Novice; // Initialize with default value
    bUseEliteInCombatOnly = true;
    EliteActivationDistance = 2000.0f;
    bEliteSystemsActive = false;

    // Initialize state
    CurrentPatrolState = EPatrolState::Patrolling;
    PatrolCenter = FVector::ZeroVector;
    CurrentPatrolTarget = FVector::ZeroVector;
    PatrolAngle = 0.0f;
    bClockwisePatrol = true;
    bIsPatrolling = false;

    // Detection state
    DetectedPlayer = nullptr;
    CurrentDetectionType = EDetectionType::None;
    LastKnownPlayerLocation = FVector::ZeroVector;
    TimeSinceLastDetection = 0.0f;

    // Investigation state
    InvestigationTarget = FVector::ZeroVector;
    InvestigationTimeRemaining = 0.0f;
    bIsInvestigating = false;

    // Combat state
    CombatTarget = nullptr;
    CombatStartTime = 0.0f;
    ConsecutiveHits = 0;
    ConsecutiveMisses = 0;

    // Portal defense
    PortalTarget = nullptr;

    // AI Overlord integration
    OverlordManager = nullptr;
    LODManager = nullptr;
    AIUnitID = -1;
}

void APortalDefenseAIController::BeginPlay()
{
    Super::BeginPlay();

    // Initialize Elite Intelligence
    InitializeEliteIntelligence();

    // Find Portal Target
    FindPortalTarget();

    // Setup Portal Defense
    SetupPortalDefense();

    // Register with managers
    RegisterWithOverlord();
    RegisterWithLODManager();

    // Start elite update timer if elite mode is enabled
    if (bEnableEliteMode && GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(EliteUpdateTimer, this,
            &APortalDefenseAIController::UpdateEliteSystemsActivation, 1.0f, true);
    }
}

void APortalDefenseAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unregister from managers
    if (OverlordManager) {
        OverlordManager->UnregisterAI(this);
    }

    if (LODManager) {
        LODManager->UnregisterAI(this);
    }

    // Clear timers
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(PatrolTimer);
        GetWorld()->GetTimerManager().ClearTimer(InvestigationTimer);
        GetWorld()->GetTimerManager().ClearTimer(EliteUpdateTimer);
    }

    Super::EndPlay(EndPlayReason);
}

void APortalDefenseAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update time since last detection
    if (DetectedPlayer) {
        TimeSinceLastDetection += DeltaTime;
    }

    // Update investigation timer
    if (bIsInvestigating && InvestigationTimeRemaining > 0.0f) {
        InvestigationTimeRemaining -= DeltaTime;
        if (InvestigationTimeRemaining <= 0.0f) {
            bIsInvestigating = false;
            ResumePatrol();
        }
    }
}

void APortalDefenseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (InPawn && EliteIntelligence) {
        // Initialize elite intelligence with possessed pawn
        EliteIntelligence->StartPlayerTracking(GetWorld()->GetFirstPlayerController()->GetPawn());
    }
}

// Implementation of other functions continues...
// (This would include all the function implementations from the original file)

void APortalDefenseAIController::InitializeEliteIntelligence()
{
    if (EliteIntelligence) {
        EliteIntelligence->SetEliteMode(bEnableEliteMode);
        if (bEnableEliteMode) {
            EliteIntelligence->SetDifficultyLevel(EliteDifficulty);
        }
    }
}

void APortalDefenseAIController::UpdateEliteSystemsActivation()
{
    if (!bEnableEliteMode) {
        bEliteSystemsActive = false;
        return;
    }

    bool bShouldActivate = ShouldActivateEliteSystems();

    if (bShouldActivate != bEliteSystemsActive) {
        bEliteSystemsActive = bShouldActivate;

        if (bEliteSystemsActive) {
            UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Elite systems ACTIVATED"), *GetName());
        } else {
            UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Elite systems deactivated"), *GetName());
        }
    }
}

bool APortalDefenseAIController::ShouldActivateEliteSystems() const
{
    if (!bEnableEliteMode || !GetPawn()) {
        return false;
    }

    // Check if player is within activation distance
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
        if (APawn* PlayerPawn = PC->GetPawn()) {
            float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

            if (bUseEliteInCombatOnly) {
                // Only activate in combat situations
                return DistanceToPlayer <= EliteActivationDistance && DetectedPlayer != nullptr;
            } else {
                // Activate based on distance only
                return DistanceToPlayer <= EliteActivationDistance;
            }
        }
    }

    return false;
}

// Add other function implementations here...

// Legacy Compatibility Function Implementations
void APortalDefenseAIController::ApplyAIUpgrade(const FPortalAIData& NewAIData)
{
    CurrentAIData = NewAIData;

    if (ACharacter* ControlledCharacter = GetCharacter()) {
        // Apply movement speed
        if (UCharacterMovementComponent* MovementComp = ControlledCharacter->GetCharacterMovement()) {
            MovementComp->MaxWalkSpeed = CurrentAIData.MovementSpeed;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AI %s received data upgrade"), *GetName());
}

void APortalDefenseAIController::SetPatrolCenter(const FVector& NewCenter)
{
    PatrolCenter = NewCenter;
    UE_LOG(LogTemp, Log, TEXT("AI %s patrol center set to: %s"), *GetName(), *NewCenter.ToString());
}

void APortalDefenseAIController::SetPatrolRadius(float NewRadius)
{
    CurrentAIData.PatrolRadius = FMath::Max(NewRadius, 100.0f);
    UE_LOG(LogTemp, Log, TEXT("AI %s patrol radius set to: %f"), *GetName(), NewRadius);
}

void APortalDefenseAIController::StartPatrolling()
{
    StartPatrol(PatrolCenter, CurrentAIData.PatrolRadius);
}

void APortalDefenseAIController::StopPatrolling()
{
    StopPatrol();
}

void APortalDefenseAIController::SetPortalTarget(APortalCore* NewTarget)
{
    PortalTarget = NewTarget;
    if (PortalTarget) {
        UE_LOG(LogTemp, Log, TEXT("AI %s assigned to defend portal: %s"), *GetName(), *PortalTarget->GetName());
        SetupPortalDefense();
    }
}