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
    PrimaryActorTick.bCanEverTick = true;

    // Create components
    StealthComponent = CreateDefaultSubobject<UACFStealthDetectionComponent>(TEXT("StealthComponent"));
    EliteIntelligence = CreateDefaultSubobject<UEliteAIIntelligenceComponent>(TEXT("EliteIntelligence"));

    CurrentState = EPortalAIState::Patrolling;
    bEnableEliteMode = false;
    EliteActivationDistance = 1000.0f;
}

void APortalDefenseAIController::BeginPlay()
{
    Super::BeginPlay();

    InitializeComponents();
    RegisterWithManagers();
}

void APortalDefenseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (InPawn && AIConfig.bUseACFCombatBehavior) {
        // Configure ACF Combat Behavior Component
        if (UACFCombatBehaviourComponent* CombatComp = InPawn->FindComponentByClass<UACFCombatBehaviourComponent>()) {
            CombatComp->DefaultCombatBehaviorType = AIConfig.PreferredCombatType;
            CombatComp->DefaultCombatState = (AIConfig.PreferredCombatType == ECombatBehaviorType::EMelee)
                ? EAICombatState::EMeleeCombat
                : EAICombatState::ERangedCombat;
        }
    }

    // Start patrolling if portal target is set
    if (PortalTarget) {
        StartPatrolling(PortalTarget->GetActorLocation(), AIConfig.PatrolRadius);
    }
}

void APortalDefenseAIController::OnUnPossess()
{
    // Unregister from LOD Manager
    if (LODManager) {
        LODManager->UnregisterAI(this);
    }

    // Clear timers
    GetWorld()->GetTimerManager().ClearTimer(PatrolTimer);
    GetWorld()->GetTimerManager().ClearTimer(InvestigationTimer);

    Super::OnUnPossess();
}

void APortalDefenseAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateEliteSystemsActivation();
}

void APortalDefenseAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (LODManager) {
        LODManager->UnregisterAI(this);
    }

    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    Super::EndPlay(EndPlayReason);
}

void APortalDefenseAIController::SetPortalTarget(APortalCore* NewTarget)
{
    PortalTarget = NewTarget;

    if (PortalTarget && GetPawn()) {
        StartPatrolling(PortalTarget->GetActorLocation(), AIConfig.PatrolRadius);
    }
}

void APortalDefenseAIController::StartPatrolling(FVector Center, float Radius)
{
    PatrolCenter = Center;
    AIConfig.PatrolRadius = Radius;
    CurrentState = EPortalAIState::Patrolling;

    GeneratePatrolPoints();
    MoveToNextPatrolPoint();
}

void APortalDefenseAIController::InvestigateLocation(FVector Location)
{
    CurrentState = EPortalAIState::Investigating;
    MoveToLocation(Location);

    // Return to patrol after investigation
    GetWorld()->GetTimerManager().SetTimer(InvestigationTimer, [this]() {
        if (CurrentState == EPortalAIState::Investigating)
        {
            StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
        } }, 8.0f, false);
}

void APortalDefenseAIController::OnPlayerDetected(APawn* Player)
{
    if (!Player)
        return;

    DetectedPlayer = Player;
    LastKnownPlayerLocation = Player->GetActorLocation();
    LastPlayerDetectionTime = GetWorld()->GetTimeSeconds();
    CurrentState = EPortalAIState::ChasingPlayer;

    SetFocus(Player);

    // Force high LOD
    if (LODManager) {
        LODManager->ForceMaximumLOD(this, 20.0f);
    }

    // Let ACF Combat Behavior Component handle combat logic
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>()) {
        // ACF handles target setting internally
    }

    UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Player detected"), *GetName());
}

void APortalDefenseAIController::OnPlayerLost()
{
    if (DetectedPlayer) {
        // Record for elite AI
        if (IsEliteModeActive()) {
            EliteIntelligence->RecordPlayerAction(DetectedPlayer, LastKnownPlayerLocation, "Escaped");
        }
    }

    DetectedPlayer = nullptr;
    ClearFocus(EAIFocusPriority::Gameplay);
    CurrentState = EPortalAIState::Returning;

    // Return to patrol
    StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
}

void APortalDefenseAIController::UpdatePatrolLogic()
{
    if (CurrentState == EPortalAIState::Patrolling) {
        MoveToNextPatrolPoint();
    }
}

void APortalDefenseAIController::UpdateCombatBehavior()
{
    // Update combat state based on ACF Combat Behavior Component
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // This integrates with ACF's combat system
        bool bWasInCombat = IsInCombat();
        // Note: ACF's IsInCombat() method may not exist, using our own logic
        bool bNowInCombat = (DetectedPlayer != nullptr);

        if (!bWasInCombat && bNowInCombat) {
            // Entered combat - force high LOD
            if (LODManager) {
                LODManager->ForceMaximumLOD(this, 30.0f);
            }
        }
    }
}

void APortalDefenseAIController::UpdateTargeting()
{
    // Update targeting state for LOD system
    if (GetFocusActor()) {
        if (APawn* FocusPawn = Cast<APawn>(GetFocusActor())) {
            if (FocusPawn->IsPlayerControlled()) {
                DetectedPlayer = FocusPawn;
                LastKnownPlayerLocation = FocusPawn->GetActorLocation();
                LastPlayerDetectionTime = GetWorld()->GetTimeSeconds();
            }
        }
    }
}

bool APortalDefenseAIController::IsInCombat() const
{
    return DetectedPlayer && IsValid(DetectedPlayer) && (GetWorld()->GetTimeSeconds() - LastPlayerDetectionTime) < 5.0f;
}

bool APortalDefenseAIController::IsEngagingPlayer() const
{
    return DetectedPlayer && IsValid(DetectedPlayer) && (GetWorld()->GetTimeSeconds() - LastPlayerDetectionTime) < 5.0f;
}

void APortalDefenseAIController::SetEliteMode(bool bEnabled)
{
    bEnableEliteMode = bEnabled;

    if (EliteIntelligence) {
        EliteIntelligence->SetEliteMode(bEnabled);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Elite mode %s"),
        *GetName(), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

bool APortalDefenseAIController::IsEliteModeActive() const
{
    return bEnableEliteMode && bEliteSystemsActive && EliteIntelligence && EliteIntelligence->IsEliteModeEnabled();
}

void APortalDefenseAIController::TriggerACFCombatAction(EAICombatState CombatState)
{
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // Let ACF handle action selection based on combat state
        // Note: TriggerActionByCombatState may not exist, handle gracefully
    }
}

EAICombatState APortalDefenseAIController::GetCurrentACFCombatState() const
{
    if (UACFCombatBehaviourComponent* CombatComp = GetPawn() ? GetPawn()->FindComponentByClass<UACFCombatBehaviourComponent>() : nullptr) {
        // Note: GetCurrentCombatState may not exist, return default
        return EAICombatState::EMeleeCombat;
    }
    return EAICombatState::EMeleeCombat;
}

void APortalDefenseAIController::ReceiveOverlordCommand(const FString& Command)
{
    if (Command == "AlertAll") {
        if (LODManager) {
            LODManager->ForceHighLOD(this, 30.0f);
        }
    } else if (Command == "ReturnToPatrol") {
        CurrentState = EPortalAIState::Returning;
        StartPatrolling(PatrolCenter, AIConfig.PatrolRadius);
    } else if (Command == "HuntPlayer") {
        if (DetectedPlayer) {
            CurrentState = EPortalAIState::ChasingPlayer;
            MoveToLocation(LastKnownPlayerLocation);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AI received command: %s"), *Command);
}

void APortalDefenseAIController::ReportToOverlord(const FString& ReportType, const FVector& Location)
{
    // Basic implementation - expand as needed
    UE_LOG(LogTemp, Log, TEXT("AI reports: %s at location %s"), *ReportType, *Location.ToString());
}

void APortalDefenseAIController::InitializeComponents()
{
    if (EliteIntelligence) {
        EliteIntelligence->SetEliteMode(bEnableEliteMode);
    }
}

void APortalDefenseAIController::RegisterWithManagers()
{
    // Find and register with LOD Manager
    LODManager = UAILODManager::GetInstance(GetWorld());
    if (LODManager) {
        LODManager->RegisterAI(this);
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Could not find AI LOD Manager"), *GetName());
    }
}

void APortalDefenseAIController::GeneratePatrolPoints()
{
    PatrolPoints.Empty();

    if (AIConfig.PatrolRadius <= 0.0f || PatrolCenter.IsZero()) {
        return;
    }

    // Generate 6 patrol points around center
    const int32 NumPoints = 6;
    for (int32 i = 0; i < NumPoints; i++) {
        float Angle = (2.0f * PI * i) / NumPoints;
        FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * AIConfig.PatrolRadius;
        PatrolPoints.Add(PatrolCenter + Offset);
    }

    CurrentPatrolIndex = 0;
}

void APortalDefenseAIController::MoveToNextPatrolPoint()
{
    if (PatrolPoints.Num() == 0 || CurrentState != EPortalAIState::Patrolling) {
        return;
    }

    FVector TargetPoint = PatrolPoints[CurrentPatrolIndex];
    MoveToLocation(TargetPoint);

    // Set timer for next point
    GetWorld()->GetTimerManager().SetTimer(PatrolTimer, [this]() {
        CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
        MoveToNextPatrolPoint(); }, 5.0f, false);
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
        }
    }
}

bool APortalDefenseAIController::ShouldActivateEliteSystems() const
{
    if (!bEnableEliteMode || !GetPawn())
        return false;

    // Activate if in combat
    if (IsInCombat())
        return true;

    // Activate if player detected and within range
    if (DetectedPlayer && IsValid(DetectedPlayer)) {
        float Distance = FVector::Dist(GetPawn()->GetActorLocation(), DetectedPlayer->GetActorLocation());
        return Distance <= EliteActivationDistance;
    }

    return false;
}