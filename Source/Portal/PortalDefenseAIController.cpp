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
    EliteDifficulty = EEliteDifficultyLevel::Novice;
    bUseEliteInCombatOnly = true;
    EliteActivationRange = 1500.0f;
    bEliteSystemsActive = false;

    // Patrol State
    CurrentPatrolState = EPatrolState::Patrolling;
    PatrolAngle = 0.0f;
    bClockwisePatrol = FMath::RandBool();
    CurrentPatrolTarget = FVector::ZeroVector;

    // Detection State
    DetectedPlayer = nullptr;
    LastDetectionType = EDetectionType::None;
    PlayerDetectionTime = 0.0f;
    bPlayerSpottedInLight = false;
    bInvestigatingSound = false;
    LastKnownPlayerLocation = FVector::ZeroVector;
    TimeSinceLastPlayerSighting = 0.0f;

    // Combat State
    EngagementStartTime = 0.0f;
    TotalEngagementTime = 0.0f;
    bIsEngagingPlayer = false;
    bInCombatState = false;
    LastAttackTime = 0.0f;
    ConsecutiveHits = 0;
    ConsecutiveMisses = 0;
    LastDodgeTime = 0.0f;
    LastDodgeDirection = FVector::ZeroVector;
    AIUnitID = FMath::RandRange(1000, 9999);

    // Set team to portal defenders
    CombatTeam = ETeam::ETeam2;

    // Set defensive behavior
    bIsAggressive = true;
    bShouldReactOnHit = true;

    // Proper ACF Action Tags - Using ACF system tags
    AttackActionTag = FGameplayTag::RequestGameplayTag(FName("Action.DefaultAttack"));
    PatrolActionTag = FGameplayTag::RequestGameplayTag(FName("Action.Walk"));
    AlertActionTag = FGameplayTag::RequestGameplayTag(FName("Action.Alert"));
    EquipWeaponActionTag = FGameplayTag::RequestGameplayTag(FName("Action.EquipWeapon"));
    UnequipWeaponActionTag = FGameplayTag::RequestGameplayTag(FName("Action.UnequipWeapon"));
    DodgeActionTag = FGameplayTag::RequestGameplayTag(FName("Action.Dodge"));
    FlankActionTag = FGameplayTag::RequestGameplayTag(FName("Action.Flank"));
    RetreatActionTag = FGameplayTag::RequestGameplayTag(FName("Action.Retreat"));
    CounterAttackActionTag = FGameplayTag::RequestGameplayTag(FName("Action.CounterAttack"));

    // Portal defense settings
    MaxChaseDistance = 2000.0f;
    PlayerThreatMultiplier = 5.0f;
    InvestigationDuration = 10.0f;
    ReturnToPatrolDelay = 3.0f;

    // Environment settings
    CoverDetectionRadius = 800.0f;
    EnvironmentAnalysisInterval = 5.0f;
    LastEnvironmentAnalysisTime = 0.0f;

    // Combat state management
    LastCombatState = EAICombatState::EIdle;
    CombatStateChangeTime = 0.0f;

    // Combat statistics
    TotalCombatEncounters = 0;
    SuccessfulCombatEncounters = 0;
    AverageCombatDuration = 0.0f;
    TotalCombatTime = 0.0f;

    // Reserve arrays for performance
    KnownCoverPositions.Reserve(20);
    KnownFlankingPositions.Reserve(10);
}

void APortalDefenseAIController::BeginPlay()
{
    Super::BeginPlay();

    InitializeEliteIntelligence();
    FindPortalTarget();
    SetupPortalDefense();
    RegisterWithOverlord();
    RegisterWithLODManager();

    // Start behavior timers with random offsets to prevent frame spikes
    float RandomOffset = FMath::RandRange(0.0f, 0.5f);
    GetWorldTimerManager().SetTimer(PatrolUpdateTimer, this, &APortalDefenseAIController::OnPatrolUpdateTimer, 0.5f + RandomOffset, true);
    GetWorldTimerManager().SetTimer(CombatUpdateTimer, this, &APortalDefenseAIController::OnCombatUpdateTimer, 0.1f + RandomOffset, true);
    GetWorldTimerManager().SetTimer(OverlordUpdateTimer, this, &APortalDefenseAIController::OnOverlordUpdateTimer, 2.0f + RandomOffset, true);
    GetWorldTimerManager().SetTimer(EnvironmentAnalysisTimer, this, &APortalDefenseAIController::OnEnvironmentAnalysisTimer, EnvironmentAnalysisInterval + RandomOffset, true);

    StartPatrolling();

    UE_LOG(LogTemp, Log, TEXT("Portal Defense AI %s initialized - Elite Mode: %s, Difficulty: %d"),
        *GetName(), bEnableEliteMode ? TEXT("Enabled") : TEXT("Disabled"), static_cast<int32>(EliteDifficulty));
}

void APortalDefenseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (InPawn) {
        OwnerPawn = InPawn;
        ApplyAIUpgrade(CurrentAIData);

        // Set patrol center around spawn location if not set
        if (PatrolCenter.IsZero()) {
            SetPatrolCenter(InPawn->GetActorLocation());
        }

        // Initialize elite systems if enabled
        if (bEnableEliteMode && EliteIntelligence) {
            EliteIntelligence->SetEliteMode(true);
            EliteIntelligence->SetDifficultyLevel(EliteDifficulty);
        }
    }
}

void APortalDefenseAIController::OnUnPossess()
{
    CleanupExpiredTimers();
    Super::OnUnPossess();
}

void APortalDefenseAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CleanupExpiredTimers();

    // Unregister from managers
    if (OverlordManager) {
        OverlordManager->UnregisterAI(this);
    }

    if (LODManager) {
        LODManager->UnregisterAI(this);
    }

    Super::EndPlay(EndPlayReason);
}

void APortalDefenseAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsEngagingPlayer) {
        TotalEngagementTime += DeltaTime;
        UpdateCombatAccuracy(TotalEngagementTime);
    }

    UpdateEliteSystemsActivation();
    CheckForPlayerThreats();
}

// LOD System Functions (Required by AI LOD Manager)
void APortalDefenseAIController::UpdatePatrolLogic()
{
    if (CurrentPatrolState != EPatrolState::Patrolling || !GetPawn()) {
        return;
    }

    const FVector CurrentLocation = GetPawn()->GetActorLocation();
    const FVector TargetLocation = CurrentPatrolTarget;

    // Check if we reached patrol point
    const float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);
    if (DistanceToTarget < 150.0f) {
        CalculateNextPatrolPoint();
    }

    // Move to patrol point
    if (TargetLocation != FVector::ZeroVector) {
        MoveToLocation(TargetLocation, 50.0f);
        SetTargetLocationBK(TargetLocation);
    }

    // Trigger patrol action if using ACF
    if (CurrentAIData.bUseACFActions && PatrolActionTag.IsValid()) {
        TriggerPatrolAction();
    }
}

void APortalDefenseAIController::UpdateCombatBehavior()
{
    if (!GetPawn() || !IsInCombat()) {
        return;
    }

    // Elite AI takes priority over standard combat behavior
    if (IsEliteModeActive() && DetectedPlayer && bEliteSystemsActive) {
        ProcessEliteCombatDecision();
        return;
    }

    // Standard ACF combat behavior
    if (UACFCombatBehaviourComponent* CombatComp = GetCombatBehaviorComponent()) {
        float TargetDistance = 0.0f;
        if (DetectedPlayer) {
            TargetDistance = FVector::Dist(GetPawn()->GetActorLocation(), DetectedPlayer->GetActorLocation());
        }

        // Get best combat state for distance
        EAICombatState BestCombatState = CombatComp->GetBestCombatStateByTargetDistance(TargetDistance);

        // Update combat state if changed
        if (BestCombatState != LastCombatState) {
            SetCombatStateBK(BestCombatState);
            LastCombatState = BestCombatState;
            CombatStateChangeTime = GetWorld()->GetTimeSeconds();
        }

        // Try to execute combat actions
        CombatComp->TryExecuteActionByCombatState(BestCombatState);
        CombatComp->TryExecuteConditionAction();
    }

    // Handle player engagement
    if (bIsEngagingPlayer && DetectedPlayer) {
        ExecuteStandardCombatBehavior();
    }
}

void APortalDefenseAIController::UpdateTargeting()
{
    if (!GetPawn()) {
        return;
    }

    // Elite AI targeting takes priority
    if (IsEliteModeActive() && bEliteSystemsActive) {
        if (EliteIntelligence && DetectedPlayer) {
            EliteIntelligence->UpdateTargeting();
        }
        return;
    }

    // Use ACF Targeting Component if available
    if (UATSAITargetComponent* TargetComp = GetTargetingComponent()) {
        TargetComp->UpdateTargeting();
    }

    // Use ACF Threat Manager to find best target
    if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
        if (AActor* BestTarget = ThreatManager->GetCurrentTarget()) {
            SetTargetActorBK(BestTarget);

            if (APawn* TargetPawn = Cast<APawn>(BestTarget)) {
                if (TargetPawn != DetectedPlayer) {
                    OnPlayerDetected(TargetPawn, EDetectionType::Visual);
                }
            }
        }
    }

    // Fallback - simple player detection
    if (!DetectedPlayer) {
        PerformBasicPlayerDetection();
    }
}

bool APortalDefenseAIController::IsInCombat() const
{
    return bInCombatState || bIsEngagingPlayer || DetectedPlayer != nullptr;
}

bool APortalDefenseAIController::IsEngagingPlayer() const
{
    return bIsEngagingPlayer;
}

// Elite AI Integration Functions
void APortalDefenseAIController::SetEliteMode(bool bEnabled, EEliteDifficultyLevel Difficulty)
{
    bEnableEliteMode = bEnabled;
    EliteDifficulty = Difficulty;

    if (EliteIntelligence) {
        EliteIntelligence->SetEliteMode(bEnabled);
        if (bEnabled) {
            EliteIntelligence->SetDifficultyLevel(Difficulty);
        }
    }

    UpdateEliteSystemsActivation();

    UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Elite Mode %s at Difficulty %d"),
        *GetName(), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"), static_cast<int32>(Difficulty));
}

bool APortalDefenseAIController::IsEliteModeActive() const
{
    return EliteIntelligence && EliteIntelligence->IsEliteModeEnabled() && bEliteSystemsActive;
}

EEliteDifficultyLevel APortalDefenseAIController::GetEliteDifficulty() const
{
    return EliteIntelligence ? EliteIntelligence->GetCurrentDifficulty() : EEliteDifficultyLevel::Disabled;
}

void APortalDefenseAIController::SetEliteDifficultyLevel(EEliteDifficultyLevel NewDifficulty)
{
    EliteDifficulty = NewDifficulty;
    if (EliteIntelligence) {
        EliteIntelligence->SetDifficultyLevel(NewDifficulty);
    }
}

bool APortalDefenseAIController::ShouldExecuteEliteDodge(const FVector& ThreatDirection, float ThreatSpeed)
{
    if (!IsEliteModeActive()) {
        return false;
    }

    return EliteIntelligence->ShouldDodgeNow(ThreatDirection, ThreatSpeed);
}

FVector APortalDefenseAIController::GetEliteOptimalPosition(APawn* Target)
{
    if (!IsEliteModeActive() || !Target) {
        return FVector::ZeroVector;
    }

    // Use elite intelligence to determine optimal position
    FVector FlankPosition = EliteIntelligence->GetOptimalFlankingPosition(Target);

    // If elite AI suggests tactical retreat
    if (EliteIntelligence->ShouldExecuteTacticalRetreat()) {
        FVector RetreatPosition = EliteIntelligence->GetPlayerBaitPosition();
        return RetreatPosition;
    }

    return FlankPosition;
}

bool APortalDefenseAIController::ShouldExecuteEliteAttack(APawn* Target)
{
    if (!IsEliteModeActive() || !Target) {
        return false;
    }

    return EliteIntelligence->ShouldAttackNow(Target);
}

void APortalDefenseAIController::RecordPlayerCombatAction(const FString& ActionType, const FVector& ActionLocation)
{
    if (IsEliteModeActive() && DetectedPlayer) {
        EliteIntelligence->RecordPlayerAction(DetectedPlayer, ActionLocation, ActionType);
    }
}

void APortalDefenseAIController::ExecuteEliteTacticalPlan(APawn* Target)
{
    if (!IsEliteModeActive() || !Target) {
        return;
    }

    FEliteTacticalPlan TacticalPlan = EliteIntelligence->GenerateTacticalPlan(Target);
    if (TacticalPlan.PlannedPositions.Num() > 0) {
        EliteIntelligence->ExecuteTacticalPlan(TacticalPlan);
    }
}

// Core AI Functions
UACFActionsManagerComponent* APortalDefenseAIController::GetActionsManager() const
{
    if (GetPawn()) {
        return GetPawn()->FindComponentByClass<UACFActionsManagerComponent>();
    }
    return nullptr;
}

void APortalDefenseAIController::SetPortalTarget(APortalCore* NewPortalTarget)
{
    PortalTarget = NewPortalTarget;
    SetupPortalDefense();
}

void APortalDefenseAIController::SetPatrolCenter(FVector Center)
{
    PatrolCenter = Center;
    CalculateNextPatrolPoint();
}

void APortalDefenseAIController::SetPatrolRadius(float Radius)
{
    CurrentAIData.PatrolRadius = Radius;
    CalculateNextPatrolPoint();
}

void APortalDefenseAIController::StartPatrolling()
{
    CurrentPatrolState = EPatrolState::Patrolling;
    DetectedPlayer = nullptr;
    bInvestigatingSound = false;
    bIsEngagingPlayer = false;
    bInCombatState = false;
    LastDetectionType = EDetectionType::None;

    // Clear any return to patrol timer
    GetWorldTimerManager().ClearTimer(ReturnToPatrolTimer);

    // Set AI state to patrol using ACF
    SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EPatrol));
    CalculateNextPatrolPoint();

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Started patrolling"), *GetName());
}

void APortalDefenseAIController::StopPatrolling()
{
    CurrentPatrolState = EPatrolState::Investigating;
    GetWorldTimerManager().ClearTimer(PatrolUpdateTimer);
}

void APortalDefenseAIController::ApplyAIUpgrade(const FPortalAIData& NewAIData)
{
    CurrentAIData = NewAIData;

    if (ACharacter* ControlledCharacter = GetCharacter()) {
        // Apply movement speed
        if (UACFCharacterMovementComponent* MovementComp = Cast<UACFCharacterMovementComponent>(ControlledCharacter->GetCharacterMovement())) {
            MovementComp->MaxWalkSpeed = CurrentAIData.MovementSpeed;
        }

        // Apply stat modifiers through ARS
        if (UARSStatisticsComponent* StatsComp = ControlledCharacter->FindComponentByClass<UARSStatisticsComponent>()) {
            FAttributesSetModifier Modifier;
            Modifier.Guid = FGuid::NewGuid();

            FAttributeModifier SpeedMod;
            SpeedMod.AttributeType = FGameplayTag::RequestGameplayTag(FName("RPG.Parameters.MovementSpeed"));
            SpeedMod.ModType = EModifierType::EAdditive;
            SpeedMod.Value = CurrentAIData.MovementSpeed - BaseAIData.MovementSpeed;
            Modifier.AttributesMod.Add(SpeedMod);

            if (Modifier.AttributesMod.Num() > 0) {
                StatsComp->AddAttributeSetModifier(Modifier);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Applied upgrade - Speed: %.1f, Detection: %.1f, Aggression: %.1f"),
        *GetName(), CurrentAIData.MovementSpeed, CurrentAIData.PlayerDetectionRange, CurrentAIData.AggressionLevel);
}

void APortalDefenseAIController::CheckForPlayerThreats()
{
    if (!GetPawn() || !GetWorld()) {
        return;
    }

    // Use ACF Threat Manager for threat detection
    if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
        if (AActor* CurrentThreat = ThreatManager->GetCurrentTarget()) {
            if (APawn* ThreatPawn = Cast<APawn>(CurrentThreat)) {
                if (ThreatPawn != DetectedPlayer) {
                    OnPlayerDetected(ThreatPawn, EDetectionType::Visual);
                }
            }
        }
    }

    // Update engagement state
    if (DetectedPlayer) {
        float DistanceToPlayer = CalculateDistanceToPlayer();

        if (DistanceToPlayer > MaxChaseDistance) {
            OnPlayerLost();
        } else if (DistanceToPlayer <= CurrentAIData.AttackRange && !bIsEngagingPlayer) {
            bIsEngagingPlayer = true;
            bInCombatState = true;
            EngagementStartTime = GetWorld()->GetTimeSeconds();

            // Set combat state using ACF
            SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));

            // Record combat encounter
            TotalCombatEncounters++;
        }
    }

    // Update time since last player sighting
    if (!DetectedPlayer) {
        TimeSinceLastPlayerSighting += GetWorld()->GetDeltaSeconds();
    } else {
        TimeSinceLastPlayerSighting = 0.0f;
        LastKnownPlayerLocation = DetectedPlayer->GetActorLocation();
    }
}

// ACF Integration Functions
void APortalDefenseAIController::TriggerAttackAction()
{
    UACFActionsManagerComponent* ActionsManager = GetActionsManager();

    if (!CurrentAIData.bUseACFActions || !ActionsManager || !AttackActionTag.IsValid()) {
        return;
    }

    if (ActionsManager->CanExecuteAction(AttackActionTag)) {
        ActionsManager->TriggerAction(AttackActionTag, EActionPriority::EHigh);

        // Record attack for elite AI learning
        if (IsEliteModeActive() && DetectedPlayer) {
            RecordPlayerCombatAction(TEXT("Attack"), DetectedPlayer->GetActorLocation());
        }
    }
}

void APortalDefenseAIController::TriggerPatrolAction()
{
    UACFActionsManagerComponent* ActionsManager = GetActionsManager();

    if (!CurrentAIData.bUseACFActions || !ActionsManager || !PatrolActionTag.IsValid()) {
        return;
    }

    if (ActionsManager->CanExecuteAction(PatrolActionTag)) {
        ActionsManager->TriggerAction(PatrolActionTag, EActionPriority::ELow);
    }
}

void APortalDefenseAIController::TriggerAlertAction()
{
    UACFActionsManagerComponent* ActionsManager = GetActionsManager();

    if (!CurrentAIData.bUseACFActions || !ActionsManager || !AlertActionTag.IsValid()) {
        return;
    }

    if (ActionsManager->CanExecuteAction(AlertActionTag)) {
        ActionsManager->TriggerAction(AlertActionTag, EActionPriority::EMedium);
    }
}

void APortalDefenseAIController::SetCombatMode(bool bEnableCombat)
{
    bInCombatState = bEnableCombat;

    if (bEnableCombat) {
        SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));

        // Equip weapon if needed
        if (EquipWeaponActionTag.IsValid()) {
            TriggerEliteAction(EquipWeaponActionTag, EActionPriority::EHigh);
        }
    } else {
        SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EPatrol));
        bIsEngagingPlayer = false;
        DetectedPlayer = nullptr;
    }
}

void APortalDefenseAIController::TriggerEliteAction(const FGameplayTag& ActionTag, EActionPriority Priority)
{
    if (!ValidateEliteAction(ActionTag)) {
        return;
    }

    UACFActionsManagerComponent* ActionsManager = GetActionsManager();
    if (ActionsManager && ActionsManager->CanExecuteAction(ActionTag)) {
        ActionsManager->TriggerAction(ActionTag, Priority);

        UE_LOG(LogTemp, Verbose, TEXT("Portal AI %s: Triggered elite action %s"),
            *GetName(), *ActionTag.ToString());
    }
}

// Advanced Combat Behaviors
void APortalDefenseAIController::ExecuteFlankingManeuver(APawn* Target)
{
    if (!Target || !GetPawn()) {
        return;
    }

    FVector FlankingPosition;

    if (IsEliteModeActive()) {
        FlankingPosition = EliteIntelligence->GetOptimalFlankingPosition(Target);
    } else {
        // Basic flanking logic
        FVector ToTarget = (Target->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal();
        FVector RightVector = FVector::CrossProduct(ToTarget, FVector::UpVector);
        bool bFlankRight = FMath::RandBool();
        FlankingPosition = Target->GetActorLocation() + (bFlankRight ? RightVector : -RightVector) * 600.0f;
    }

    SetTargetLocationBK(FlankingPosition);
    MoveToLocation(FlankingPosition, 100.0f);

    if (FlankActionTag.IsValid()) {
        TriggerEliteAction(FlankActionTag, EActionPriority::EMedium);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Executing flanking maneuver"), *GetName());
}

void APortalDefenseAIController::ExecuteTacticalRetreat()
{
    if (!GetPawn()) {
        return;
    }

    FVector RetreatPosition;

    if (IsEliteModeActive() && EliteIntelligence->ShouldExecuteTacticalRetreat()) {
        RetreatPosition = EliteIntelligence->GetPlayerBaitPosition();
    } else {
        // Basic retreat logic
        FVector ToPlayer = DetectedPlayer ? (DetectedPlayer->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal() : FVector::ForwardVector;
        RetreatPosition = GetPawn()->GetActorLocation() - (ToPlayer * 800.0f);
    }

    SetTargetLocationBK(RetreatPosition);
    MoveToLocation(RetreatPosition, 100.0f);

    if (RetreatActionTag.IsValid()) {
        TriggerEliteAction(RetreatActionTag, EActionPriority::EHigh);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Executing tactical retreat"), *GetName());
}

void APortalDefenseAIController::ExecuteAdvancedDodge(const FVector& ThreatDirection, const FVector& ThreatVelocity)
{
    if (!GetPawn()) {
        return;
    }

    FVector DodgeDirection;

    if (IsEliteModeActive()) {
        DodgeDirection = EliteIntelligence->GetOptimalDodgeDirection(ThreatDirection);
    } else {
        // Basic perpendicular dodge
        DodgeDirection = FVector::CrossProduct(ThreatDirection, FVector::UpVector);
        if (FMath::RandBool()) {
            DodgeDirection = -DodgeDirection;
        }
    }

    FVector DodgePosition = GetPawn()->GetActorLocation() + (DodgeDirection * 400.0f);
    SetTargetLocationBK(DodgePosition);
    MoveToLocation(DodgePosition, 50.0f);

    // Record dodge for learning
    LastDodgeTime = GetWorld()->GetTimeSeconds();
    LastDodgeDirection = DodgeDirection;

    if (DodgeActionTag.IsValid()) {
        TriggerEliteAction(DodgeActionTag, EActionPriority::EHigh);
    }

    if (IsEliteModeActive()) {
        RecordPlayerCombatAction(TEXT("Dodge"), DodgePosition);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Executed advanced dodge"), *GetName());
}

void APortalDefenseAIController::ExecutePredictiveAttack(APawn* Target)
{
    if (!Target || !GetPawn()) {
        return;
    }

    FVector PredictedPosition;

    if (IsEliteModeActive()) {
        PredictedPosition = EliteIntelligence->PredictPlayerPosition(0.5f);
    } else {
        // Basic prediction
        PredictedPosition = Target->GetActorLocation() + (Target->GetVelocity() * 0.5f);
    }

    // Aim at predicted position
    FVector DirectionToPredicted = (PredictedPosition - GetPawn()->GetActorLocation()).GetSafeNormal();
    FRotator TargetRotation = DirectionToPredicted.Rotation();
    SetControlRotation(TargetRotation);

    // Execute attack
    TriggerAttackAction();

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Executed predictive attack"), *GetName());
}

void APortalDefenseAIController::ExecuteCounterAttack(APawn* Target)
{
    if (!Target || !GetPawn()) {
        return;
    }

    // Wait for optimal counter-attack window
    if (IsEliteModeActive() && !EliteIntelligence->ShouldAttackNow(Target)) {
        return;
    }

    if (CounterAttackActionTag.IsValid()) {
        TriggerEliteAction(CounterAttackActionTag, EActionPriority::EHigh);
    } else {
        TriggerAttackAction();
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Executed counter-attack"), *GetName());
}

// Detection and Investigation Functions
void APortalDefenseAIController::InvestigateLocation(FVector Location, float Duration)
{
    CurrentPatrolState = EPatrolState::Investigating;
    SetTargetLocationBK(Location);
    MoveToLocation(Location, 100.0f);

    GetWorldTimerManager().SetTimer(InvestigationTimer, this,
        &APortalDefenseAIController::OnInvestigationComplete, Duration, false);

    TriggerAlertAction();

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Investigating location %s"),
        *GetName(), *Location.ToString());
}

void APortalDefenseAIController::InvestigateSound(FVector SoundLocation, float Duration)
{
    bInvestigatingSound = true;
    CurrentPatrolState = EPatrolState::InvestigatingSound;
    SetTargetLocationBK(SoundLocation);
    MoveToLocation(SoundLocation, 100.0f);

    GetWorldTimerManager().SetTimer(SoundInvestigationTimer, this,
        &APortalDefenseAIController::OnSoundInvestigationComplete, Duration, false);

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Investigating sound at %s"),
        *GetName(), *SoundLocation.ToString());
}

void APortalDefenseAIController::OnPlayerDetected(APawn* Player, EDetectionType DetectionType)
{
    if (!Player) {
        return;
    }

    DetectedPlayer = Player;
    LastDetectionType = DetectionType;
    bIsEngagingPlayer = true;
    bInCombatState = true;
    CurrentPatrolState = EPatrolState::ChasingPlayer;
    PlayerDetectionTime = GetWorld()->GetTimeSeconds();
    LastKnownPlayerLocation = Player->GetActorLocation();

    // Elite AI learns about player detection
    if (IsEliteModeActive()) {
        RecordPlayerCombatAction(TEXT("Detection"), Player->GetActorLocation());
    }

    // Add threat through ACF system
    if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
        float ThreatLevel = PlayerThreatMultiplier * 1000.0f;

        // Elite AI adjusts threat calculation
        if (IsEliteModeActive()) {
            float EliteMultiplier = 1.0f + (static_cast<float>(GetEliteDifficulty()) * 0.1f);
            ThreatLevel *= EliteMultiplier;
        }

        ThreatManager->AddThreat(Player, ThreatLevel);
    }

    SetTargetActorBK(Player);
    SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));
    SetCombatMode(true);

    TriggerAlertAction();

    // Report to overlord
    ReportToOverlord(TEXT("PlayerDetected"), Player->GetActorLocation());

    UE_LOG(LogTemp, Warning, TEXT("Portal AI %s: Player detected at distance %.1f %s"),
        *GetName(),
        FVector::Dist(GetPawn()->GetActorLocation(), Player->GetActorLocation()),
        IsEliteModeActive() ? TEXT("(ELITE MODE)") : TEXT(""));
}

void APortalDefenseAIController::OnPlayerLost()
{
    DetectedPlayer = nullptr;
    bIsEngagingPlayer = false;
    bInCombatState = false;
    LastDetectionType = EDetectionType::None;
    CurrentPatrolState = EPatrolState::ReturningToPatrol;

    // Clear target
    SetTargetActorBK(nullptr);
    SetCombatMode(false);

    // Record combat outcome if we were in combat
    if (TotalEngagementTime > 0.0f) {
        UpdateCombatStatistics();
        RecordCombatOutcome(false, TotalEngagementTime);
        TotalEngagementTime = 0.0f;
    }

    // Return to patrol after a delay
    GetWorldTimerManager().SetTimer(ReturnToPatrolTimer, this,
        &APortalDefenseAIController::OnReturnToPatrolTimer, ReturnToPatrolDelay, false);

    // Report to overlord
    ReportToOverlord(TEXT("PlayerLost"), LastKnownPlayerLocation);

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Player lost, returning to patrol"), *GetName());
}

void APortalDefenseAIController::StartSoundInvestigation(FVector SoundLocation)
{
    InvestigateSound(SoundLocation, InvestigationDuration * 0.8f);
}

// Environmental Interaction
void APortalDefenseAIController::AnalyzeEnvironment()
{
    if (!GetPawn() || GetWorld()->GetTimeSeconds() - LastEnvironmentAnalysisTime < EnvironmentAnalysisInterval) {
        return;
    }

    LastEnvironmentAnalysisTime = GetWorld()->GetTimeSeconds();

    // Clear old data
    KnownCoverPositions.Empty();
    KnownFlankingPositions.Empty();

    FVector MyLocation = GetPawn()->GetActorLocation();

    // Trace for cover positions around AI
    for (int32 i = 0; i < 8; ++i) {
        float Angle = (360.0f / 8.0f) * i;
        FVector Direction = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)),
            FMath::Sin(FMath::DegreesToRadians(Angle)), 0.0f);
        FVector TraceEnd = MyLocation + (Direction * CoverDetectionRadius);

        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetPawn());

        if (GetWorld()->LineTraceSingleByChannel(HitResult, MyLocation, TraceEnd,
                ECC_Visibility, QueryParams)) {

            if (HitResult.bBlockingHit) {
                KnownCoverPositions.Add(HitResult.Location);

                // Potential flanking position behind cover
                FVector FlankPosition = HitResult.Location + (Direction * -200.0f);
                KnownFlankingPositions.Add(FlankPosition);
            }
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("Portal AI %s: Analyzed environment - Found %d cover positions, %d flanking positions"),
        *GetName(), KnownCoverPositions.Num(), KnownFlankingPositions.Num());
}

FVector APortalDefenseAIController::FindNearestCover(const FVector& ThreatDirection)
{
    if (KnownCoverPositions.Num() == 0) {
        AnalyzeEnvironment();
    }

    if (KnownCoverPositions.Num() == 0) {
        return FVector::ZeroVector;
    }

    FVector MyLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
    FVector BestCoverPosition = FVector::ZeroVector;
    float BestScore = -1.0f;

    for (const FVector& CoverPosition : KnownCoverPositions) {
        // Calculate score based on distance and angle to threat
        float Distance = FVector::Dist(MyLocation, CoverPosition);
        FVector ToCover = (CoverPosition - MyLocation).GetSafeNormal();
        float Alignment = FVector::DotProduct(ToCover, -ThreatDirection.GetSafeNormal());

        float Score = Alignment / (Distance * 0.001f + 1.0f); // Prefer closer cover in threat direction

        if (Score > BestScore) {
            BestScore = Score;
            BestCoverPosition = CoverPosition;
        }
    }

    return BestCoverPosition;
}

bool APortalDefenseAIController::IsPositionInCover(const FVector& Position, const FVector& ThreatDirection)
{
    FVector TraceStart = Position + FVector(0, 0, 80); // Head height
    FVector TraceEnd = TraceStart + (ThreatDirection.GetSafeNormal() * 1000.0f);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetPawn());

    return GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd,
               ECC_Visibility, QueryParams)
        && HitResult.bBlockingHit;
}

// Pattern Recognition and Learning
void APortalDefenseAIController::AnalyzePlayerBehavior()
{
    if (IsEliteModeActive()) {
        EliteIntelligence->AnalyzePlayerPatterns();
    }

    // Basic pattern analysis for non-elite AI
    // This could be expanded for base AI learning
}

void APortalDefenseAIController::AdaptCombatStrategy()
{
    if (IsEliteModeActive()) {
        EliteIntelligence->AdaptToPlayerBehavior();
    }

    // Basic adaptation based on combat statistics
    if (TotalCombatEncounters > 3) {
        float SuccessRate = static_cast<float>(SuccessfulCombatEncounters) / TotalCombatEncounters;

        if (SuccessRate < 0.3f) {
            // Increase aggression if losing too much
            CurrentAIData.AggressionLevel = FMath::Min(CurrentAIData.AggressionLevel + 0.5f, 3.0f);
            CurrentAIData.ReactionTime = FMath::Max(CurrentAIData.ReactionTime - 0.1f, 0.1f);
        }
    }
}

void APortalDefenseAIController::RecordCombatOutcome(bool bVictorious, float CombatDuration)
{
    if (bVictorious) {
        SuccessfulCombatEncounters++;
    }

    TotalCombatTime += CombatDuration;
    AverageCombatDuration = TotalCombatTime / TotalCombatEncounters;

    UE_LOG(LogTemp, Log, TEXT("Portal AI %s: Combat ended - %s, Duration: %.1fs, Success Rate: %.1f%%"),
        *GetName(), bVictorious ? TEXT("Victory") : TEXT("Defeat"), CombatDuration,
        (static_cast<float>(SuccessfulCombatEncounters) / TotalCombatEncounters) * 100.0f);
}

// Overlord System Integration
void APortalDefenseAIController::ReceiveOverlordCommand(const FString& Command, const TArray<FVector>& Parameters)
{
    if (Command == "IncreasePatrolRadius") {
        CurrentAIData.PatrolRadius = FMath::Min(CurrentAIData.PatrolRadius + 200.0f, 1000.0f);
        CalculateNextPatrolPoint();
    } else if (Command == "IncreaseAggression") {
        CurrentAIData.AggressionLevel = FMath::Min(CurrentAIData.AggressionLevel + 0.5f, 3.0f);
        CurrentAIData.PlayerDetectionRange *= 1.2f;
        ApplyAIUpgrade(CurrentAIData);
    } else if (Command == "Alert") {
        TriggerAlertAction();
        if (Parameters.Num() > 0) {
            InvestigateLocation(Parameters[0], InvestigationDuration);
        }
    } else if (Command == "IncreaseDetectionRange") {
        CurrentAIData.PlayerDetectionRange = FMath::Min(CurrentAIData.PlayerDetectionRange + 300.0f, 2000.0f);
    } else if (Command == "EnableEliteMode") {
        if (Parameters.Num() > 0) {
            int32 DifficultyLevel = FMath::RoundToInt(Parameters[0].X);
            SetEliteMode(true, static_cast<EEliteDifficultyLevel>(DifficultyLevel));
        } else {
            SetEliteMode(true, EEliteDifficultyLevel::Novice);
        }
    } else if (Command == "AdaptToPlayerRoutes") {
        // Adapt patrol routes based on player movement patterns
        if (Parameters.Num() > 0) {
            FVector AdaptedCenter = PatrolCenter + Parameters[0] * 200.0f;
            SetPatrolCenter(AdaptedCenter);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AI Unit %d received command: %s"), AIUnitID, *Command);
}

void APortalDefenseAIController::ReportToOverlord(const FString& ReportType, const FVector& Location)
{
    if (!OverlordManager) {
        return;
    }

    if (ReportType == "PlayerDetected") {
        OverlordManager->RecordPlayerPosition(Location);
        OverlordManager->RecordPlayerIncursion(Location);
    } else if (ReportType == "PlayerLost") {
        OverlordManager->RecordPlayerPosition(Location);
    } else if (ReportType == "AIDeath") {
        OverlordManager->RecordAIDeath(this, Location);
    }
}

// Private Helper Functions
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

    // Activate elite systems based on configuration
    if (bUseEliteInCombatOnly) {
        return IsInCombat();
    }

    // Activate if player is within activation range
    if (DetectedPlayer) {
        float DistanceToPlayer = CalculateDistanceToPlayer();
        return DistanceToPlayer <= EliteActivationRange;
    }

    return false;
}

void APortalDefenseAIController::ProcessEliteCombatDecision()
{
    if (!DetectedPlayer || !EliteIntelligence) {
        return;
    }

    // Elite AI attack decision
    if (EliteIntelligence->ShouldAttackNow(DetectedPlayer)) {
        HandleEliteActionSelection(DetectedPlayer);
        return;
    }

    // Elite AI movement decision
    ExecuteEliteMovementStrategy(DetectedPlayer);
}

void APortalDefenseAIController::ExecuteEliteMovementStrategy(APawn* Target)
{
    if (!Target) {
        return;
    }

    FVector ElitePosition = GetEliteOptimalPosition(Target);
    if (ElitePosition != FVector::ZeroVector) {
        SetTargetLocationBK(ElitePosition);
        MoveToLocation(ElitePosition, 100.0f);
    }
}

void APortalDefenseAIController::HandleEliteActionSelection(APawn* Target)
{
    if (!Target) {
        return;
    }

    FGameplayTag EliteAttackAction = EliteIntelligence->GetOptimalAttackAction(Target);
    if (EliteAttackAction.IsValid()) {
        TriggerEliteAction(EliteAttackAction, EActionPriority::EHigh);
        RecordPlayerCombatAction(TEXT("Attack"), Target->GetActorLocation());
    }
}

void APortalDefenseAIController::ExecuteStandardCombatBehavior()
{
    if (!DetectedPlayer || !GetPawn()) {
        return;
    }

    const float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), DetectedPlayer->GetActorLocation());

    if (DistanceToPlayer <= CurrentAIData.AttackRange) {
        // In attack range - orient and attack
        FVector DirectionToPlayer = (DetectedPlayer->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal();
        FRotator TargetRotation = DirectionToPlayer.Rotation();
        SetControlRotation(TargetRotation);

        float CurrentTime = GetWorld()->GetTimeSeconds();
        float AttackReactionTime = CurrentAIData.ReactionTime;

        if (CurrentTime - LastAttackTime >= AttackReactionTime) {
            TriggerAttackAction();
            LastAttackTime = CurrentTime;
        }
    } else {
        // Move towards player
        SetTargetLocationBK(DetectedPlayer->GetActorLocation());
        MoveToLocation(DetectedPlayer->GetActorLocation(), CurrentAIData.AttackRange * 0.8f);
    }
}

void APortalDefenseAIController::PerformBasicPlayerDetection()
{
    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
        const float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

        if (DistanceToPlayer <= CurrentAIData.PlayerDetectionRange) {
            // Line of sight check
            if (HasLineOfSightToPlayer()) {
                OnPlayerDetected(PlayerPawn, EDetectionType::Visual);
            }
        }
    }
}

void APortalDefenseAIController::PerformAdvancedThreatAssessment()
{
    // Advanced threat assessment for high-level AI
    // This could include analyzing multiple threats, environmental hazards, etc.
}

void APortalDefenseAIController::ExecuteComplexManeuver(const FString& ManeuverType, APawn* Target)
{
    if (ManeuverType == "Flank") {
        ExecuteFlankingManeuver(Target);
    } else if (ManeuverType == "Retreat") {
        ExecuteTacticalRetreat();
    } else if (ManeuverType == "Counter") {
        ExecuteCounterAttack(Target);
    }
}

void APortalDefenseAIController::UpdateCombatAdaptation()
{
    // Update combat behavior based on ongoing battle
    if (ConsecutiveMisses > 3) {
        // Improve accuracy
        CurrentAIData.AccuracyMultiplier = FMath::Min(CurrentAIData.AccuracyMultiplier + 0.1f, 1.0f);
        ConsecutiveMisses = 0;
    }

    if (ConsecutiveHits > 5) {
        // Increase aggression
        CurrentAIData.AggressionLevel = FMath::Min(CurrentAIData.AggressionLevel + 0.2f, 3.0f);
        ConsecutiveHits = 0;
    }
}

bool APortalDefenseAIController::ValidateEliteAction(const FGameplayTag& ActionTag) const
{
    if (!ActionTag.IsValid()) {
        return false;
    }

    UACFActionsManagerComponent* ActionsManager = GetActionsManager();
    return ActionsManager && ActionsManager->CanExecuteAction(ActionTag);
}

void APortalDefenseAIController::FindPortalTarget()
{
    if (!PortalTarget) {
        TArray<AActor*> FoundPortals;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortalCore::StaticClass(), FoundPortals);

        if (FoundPortals.Num() > 0) {
            PortalTarget = Cast<APortalCore>(FoundPortals[0]);
        }
    }
}

void APortalDefenseAIController::SetupPortalDefense()
{
    if (PortalTarget && PatrolCenter.IsZero()) {
        PatrolCenter = PortalTarget->GetActorLocation();
        CalculateNextPatrolPoint();
    }
}

void APortalDefenseAIController::RegisterWithOverlord()
{
    OverlordManager = UAIOverlordManager::GetInstance(GetWorld());
    if (OverlordManager) {
        OverlordManager->RegisterAI(this);
    }
}

void APortalDefenseAIController::RegisterWithLODManager()
{
    LODManager = UAILODManager::GetInstance(GetWorld());
    if (LODManager) {
        LODManager->RegisterAI(this);
    }
}

void APortalDefenseAIController::CalculateNextPatrolPoint()
{
    if (PatrolCenter.IsZero()) {
        return;
    }

    // Circular patrol pattern
    PatrolAngle += bClockwisePatrol ? 60.0f : -60.0f;
    if (PatrolAngle >= 360.0f)
        PatrolAngle -= 360.0f;
    if (PatrolAngle < 0.0f)
        PatrolAngle += 360.0f;

    float RadianAngle = FMath::DegreesToRadians(PatrolAngle);
    CurrentPatrolTarget = PatrolCenter + FVector(FMath::Cos(RadianAngle) * CurrentAIData.PatrolRadius, FMath::Sin(RadianAngle) * CurrentAIData.PatrolRadius, 0.0f);

    SetTargetLocationBK(CurrentPatrolTarget);
}

void APortalDefenseAIController::UpdateCombatAccuracy(float EngagementTime)
{
    float BaseAccuracy = 0.2f;
    float MaxAccuracy = 0.8f;
    float ImprovementRate = 2.0f;

    float ImprovedAccuracy = BaseAccuracy + (MaxAccuracy - BaseAccuracy) * (1.0f - FMath::Exp(-EngagementTime / ImprovementRate));
    CurrentAIData.AccuracyMultiplier = ImprovedAccuracy;
}

void APortalDefenseAIController::UpdateCombatStatistics()
{
    // Update internal combat statistics for learning
    TotalCombatTime += TotalEngagementTime;
    if (TotalCombatEncounters > 0) {
        AverageCombatDuration = TotalCombatTime / TotalCombatEncounters;
    }
}

// Timer Callbacks
void APortalDefenseAIController::OnPatrolUpdateTimer()
{
    if (CurrentPatrolState == EPatrolState::Patrolling) {
        UpdatePatrolLogic();
    }
}

void APortalDefenseAIController::OnCombatUpdateTimer()
{
    if (IsInCombat()) {
        UpdateCombatBehavior();
        UpdateCombatAdaptation();
    }
    UpdateTargeting();
}

void APortalDefenseAIController::OnOverlordUpdateTimer()
{
    // Communicate with overlord manager
    if (OverlordManager) {
        if (DetectedPlayer) {
            OverlordManager->RecordPlayerPosition(DetectedPlayer->GetActorLocation());
        }
    }

    // Analyze player behavior for learning
    AnalyzePlayerBehavior();
}

void APortalDefenseAIController::OnInvestigationComplete()
{
    CurrentPatrolState = EPatrolState::ReturningToPatrol;
    GetWorldTimerManager().SetTimer(ReturnToPatrolTimer, this,
        &APortalDefenseAIController::OnReturnToPatrolTimer, ReturnToPatrolDelay, false);
}

void APortalDefenseAIController::OnSoundInvestigationComplete()
{
    bInvestigatingSound = false;
    CurrentPatrolState = EPatrolState::ReturningToPatrol;
    GetWorldTimerManager().SetTimer(ReturnToPatrolTimer, this,
        &APortalDefenseAIController::OnReturnToPatrolTimer, ReturnToPatrolDelay, false);
}

void APortalDefenseAIController::OnEnvironmentAnalysisTimer()
{
    AnalyzeEnvironment();
}

void APortalDefenseAIController::OnReturnToPatrolTimer()
{
    StartPatrolling();
}

// Utility Functions
float APortalDefenseAIController::CalculateDistanceToPlayer() const
{
    if (!DetectedPlayer || !GetPawn()) {
        return 9999.0f;
    }

    return FVector::Dist(GetPawn()->GetActorLocation(), DetectedPlayer->GetActorLocation());
}

bool APortalDefenseAIController::IsPlayerInRange(float Range) const
{
    return CalculateDistanceToPlayer() <= Range;
}

bool APortalDefenseAIController::HasLineOfSightToPlayer() const
{
    if (!DetectedPlayer || !GetPawn()) {
        return false;
    }

    FHitResult HitResult;
    FVector StartLocation = GetPawn()->GetActorLocation() + FVector(0, 0, 80);
    FVector EndLocation = DetectedPlayer->GetActorLocation() + FVector(0, 0, 80);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetPawn());
    QueryParams.AddIgnoredActor(DetectedPlayer);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        QueryParams);

    return !bHit || HitResult.GetActor() == DetectedPlayer;
}

FVector APortalDefenseAIController::GetPredictedPlayerPosition(float PredictionTime) const
{
    if (!DetectedPlayer) {
        return FVector::ZeroVector;
    }

    if (IsEliteModeActive()) {
        return EliteIntelligence->PredictPlayerPosition(PredictionTime);
    }

    // Basic prediction
    return DetectedPlayer->GetActorLocation() + (DetectedPlayer->GetVelocity() * PredictionTime);
}

bool APortalDefenseAIController::IsPositionSafe(const FVector& Position) const
{
    // Check if position is safe (not exposed to multiple threats, etc.)
    // This is a simplified version
    return !IsPositionInCover(Position, FVector::ForwardVector);
}

void APortalDefenseAIController::CleanupExpiredTimers()
{
    GetWorldTimerManager().ClearTimer(PatrolUpdateTimer);
    GetWorldTimerManager().ClearTimer(CombatUpdateTimer);
    GetWorldTimerManager().ClearTimer(OverlordUpdateTimer);
    GetWorldTimerManager().ClearTimer(InvestigationTimer);
    GetWorldTimerManager().ClearTimer(SoundInvestigationTimer);
    GetWorldTimerManager().ClearTimer(EnvironmentAnalysisTimer);
    GetWorldTimerManager().ClearTimer(ReturnToPatrolTimer);
}