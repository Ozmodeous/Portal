// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "EliteAIIntelligenceComponent.h"
#include "Algo/Sort.h"
#include "Components/ACFActionsManagerComponent.h"
#include "Components/ACFCombatBehaviourComponent.h"
#include "Components/ACFThreatManagerComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Game/ACFFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

// Static difficulty presets
TMap<EEliteDifficultyLevel, FEliteDifficultySettings> UEliteAIIntelligenceComponent::DifficultyPresets;

UEliteAIIntelligenceComponent::UEliteAIIntelligenceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f; // Tick every frame for maximum responsiveness

    bEliteModeEnabled = false;
    CurrentDifficulty = EEliteDifficultyLevel::Disabled;

    RecentFrameTimes.Reserve(60); // Store last 60 frames for analysis

    LastAnalysisTime = 0.0f;
    LastPredictionTime = 0.0f;
    LastTacticalPlanTime = 0.0f;
    CombatStartTime = 0.0f;
    bInCombat = false;

    AverageFrameTime = 16.67f; // 60 FPS baseline
    PredictionCacheValidTime = 0.05f; // Cache predictions for 50ms
}

void UEliteAIIntelligenceComponent::BeginPlay()
{
    Super::BeginPlay();

    // Initialize difficulty presets if not done
    if (DifficultyPresets.Num() == 0) {
        InitializeDifficultySettings();
    }

    // Get references
    OwnerAIController = Cast<AACFAIController>(GetOwner());
    if (OwnerAIController) {
        OwnerPawn = OwnerAIController->GetPawn();
    }

    // Find player
    if (UWorld* World = GetWorld()) {
        TrackedPlayer = UGameplayStatics::GetPlayerPawn(World, 0);
    }

    UpdateDifficultySettings();
}

void UEliteAIIntelligenceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEliteModeEnabled || CurrentDifficulty == EEliteDifficultyLevel::Disabled) {
        return;
    }

    UpdateFrameTiming(DeltaTime);
    UpdatePlayerTracking();
    UpdateCombatState();

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // Pattern analysis (frequency based on difficulty)
    float AnalysisInterval = FMath::Lerp(1.0f, 0.1f, CurrentSettings.PatternLearningSpeed);
    if (CurrentTime - LastAnalysisTime >= AnalysisInterval) {
        AnalyzePlayerPatterns();
        LastAnalysisTime = CurrentTime;
    }

    // Tactical planning (higher difficulty = more frequent planning)
    float PlanningInterval = FMath::Lerp(5.0f, 0.5f, CurrentSettings.TacticalPlanningDepth);
    if (CurrentTime - LastTacticalPlanTime >= PlanningInterval && bInCombat) {
        if (TrackedPlayer) {
            CurrentTacticalPlan = GenerateTacticalPlan(TrackedPlayer);
            LastTacticalPlanTime = CurrentTime;
        }
    }

    // Execute current tactical plan
    if (CurrentTacticalPlan.bIsExecuting) {
        ExecuteTacticalPlan(CurrentTacticalPlan);
    }

    // Counter-adaptation at highest levels
    if (CurrentSettings.bCanCounterAdapt && CurrentTime - CombatStartTime > 10.0f) {
        ExecuteCounterAdaptation();
    }
}

void UEliteAIIntelligenceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up any active plans or behaviors
    CurrentTacticalPlan.bIsExecuting = false;

    Super::EndPlay(EndPlayReason);
}

// Core Elite AI Control
void UEliteAIIntelligenceComponent::SetEliteMode(bool bEnabled)
{
    bEliteModeEnabled = bEnabled;

    if (bEnabled && CurrentDifficulty == EEliteDifficultyLevel::Disabled) {
        SetDifficultyLevel(EEliteDifficultyLevel::Novice);
    } else if (!bEnabled) {
        CurrentDifficulty = EEliteDifficultyLevel::Disabled;
    }

    UpdateDifficultySettings();

    UE_LOG(LogTemp, Warning, TEXT("Elite AI Mode %s for %s"),
        bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"),
        OwnerAIController ? *OwnerAIController->GetName() : TEXT("Unknown"));
}

void UEliteAIIntelligenceComponent::SetDifficultyLevel(EEliteDifficultyLevel NewDifficulty)
{
    CurrentDifficulty = NewDifficulty;

    if (NewDifficulty == EEliteDifficultyLevel::Disabled) {
        bEliteModeEnabled = false;
    } else {
        bEliteModeEnabled = true;
    }

    UpdateDifficultySettings();

    UE_LOG(LogTemp, Warning, TEXT("Elite AI Difficulty set to %d for %s"),
        static_cast<int32>(NewDifficulty),
        OwnerAIController ? *OwnerAIController->GetName() : TEXT("Unknown"));
}

// Combat Intelligence
bool UEliteAIIntelligenceComponent::ShouldDodgeNow(const FVector& ThreatDirection, float ThreatSpeed)
{
    if (!bEliteModeEnabled || !TrackedPlayer || !OwnerPawn) {
        return false;
    }

    // Frame-perfect timing for highest difficulties
    if (CurrentSettings.bUsesFramePerfectTiming) {
        // Calculate exact frames to impact
        float DistanceToThreat = FVector::Dist(OwnerPawn->GetActorLocation(),
            TrackedPlayer->GetActorLocation());
        float TimeToImpact = DistanceToThreat / FMath::Max(ThreatSpeed, 1.0f);

        // Perfect timing: dodge exactly when needed
        float OptimalDodgeTime = AverageFrameTime * 2.0f / 1000.0f; // 2 frames in seconds

        return FMath::Abs(TimeToImpact - OptimalDodgeTime) < (AverageFrameTime / 1000.0f);
    }

    // Predictive dodging based on difficulty
    if (CurrentSettings.PredictionAccuracy > 0.0f) {
        FVector PredictedThreatPos = PredictPlayerPosition(0.5f);
        FVector ThreatToAI = OwnerPawn->GetActorLocation() - PredictedThreatPos;
        float ThreatAlignment = FVector::DotProduct(ThreatDirection.GetSafeNormal(),
            ThreatToAI.GetSafeNormal());

        // Dodge if threat is highly aligned with our position
        return ThreatAlignment > (0.7f * CurrentSettings.PredictionAccuracy);
    }

    // Basic reactive dodging
    float DodgeThreshold = FMath::Lerp(0.9f, 0.1f, CurrentSettings.DodgePerfection);
    return FMath::RandRange(0.0f, 1.0f) < DodgeThreshold;
}

FVector UEliteAIIntelligenceComponent::GetOptimalDodgeDirection(const FVector& ThreatDirection)
{
    if (!OwnerPawn || !TrackedPlayer) {
        return FVector::ZeroVector;
    }

    FVector DodgeDirection = FVector::ZeroVector;

    // Analyze player patterns for counter-dodging
    if (CurrentSettings.bCanCounterAdapt && CurrentPlayerPattern.PatternConfidence > 0.5f) {
        // Counter-dodge: move opposite to player's expected dodge direction
        DodgeDirection = -CurrentPlayerPattern.PreferredDodgeDirection;
    } else if (CurrentSettings.FlankingIntelligence > 0.0f) {
        // Strategic dodging: position for flanking opportunity
        FVector ToPlayer = (TrackedPlayer->GetActorLocation() - OwnerPawn->GetActorLocation()).GetSafeNormal();
        FVector RightVector = FVector::CrossProduct(ToPlayer, FVector::UpVector);

        // Choose left or right based on tactical advantage
        bool bDodgeRight = FMath::RandBool();
        DodgeDirection = bDodgeRight ? RightVector : -RightVector;
    } else {
        // Standard perpendicular dodge
        FVector RightVector = FVector::CrossProduct(ThreatDirection, FVector::UpVector);
        DodgeDirection = FMath::RandBool() ? RightVector : -RightVector;
    }

    // Add environment awareness at higher difficulties
    if (CurrentSettings.EnvironmentUsage > 0.0f) {
        // Prefer dodging toward cover or advantageous positions
        // This would require environment analysis - simplified for now
        DodgeDirection = CalculateAdvancedDodge(ThreatDirection, TrackedPlayer->GetVelocity());
    }

    OnEliteBehaviorTriggered.Broadcast(TEXT("OptimalDodge"), CurrentSettings.DodgePerfection);

    return DodgeDirection.GetSafeNormal();
}

bool UEliteAIIntelligenceComponent::ShouldAttackNow(APawn* Target)
{
    if (!bEliteModeEnabled || !Target || !OwnerPawn) {
        return false;
    }

    // Frame-perfect attack timing
    if (CurrentSettings.bUsesFramePerfectTiming) {
        // Attack during player's vulnerability windows
        float TimeSinceLastPlayerAction = GetWorld()->GetTimeSeconds() - (CurrentPlayerPattern.AttackTimings.Num() > 0 ? CurrentPlayerPattern.AttackTimings.Last() : 0.0f);

        // Target specific vulnerability windows
        return TimeSinceLastPlayerAction > 0.2f && TimeSinceLastPlayerAction < 0.4f;
    }

    // Predictive attacking
    if (CurrentSettings.AttackPrediction > 0.0f) {
        FVector PredictedPos = PredictPlayerPosition(0.3f);
        float PredictedDistance = FVector::Dist(OwnerPawn->GetActorLocation(), PredictedPos);

        // Attack when player will be in optimal range
        float OptimalRange = 800.0f; // Configurable based on weapon type
        return FMath::Abs(PredictedDistance - OptimalRange) < (200.0f * CurrentSettings.AttackPrediction);
    }

    // Pattern-based attacking
    if (CurrentPlayerPattern.PatternConfidence > 0.3f && CurrentSettings.PatternLearningSpeed > 0.0f) {
        // Attack based on learned player behavior patterns
        float TimeSinceLastAttack = GetWorld()->GetTimeSeconds() - CombatStartTime;
        float PredictedPlayerAttackTime = CurrentPlayerPattern.AttackFrequency;

        // Attack just before player's predicted attack
        return FMath::Abs(TimeSinceLastAttack - PredictedPlayerAttackTime) < 0.1f;
    }

    return false;
}

FVector UEliteAIIntelligenceComponent::PredictPlayerPosition(float PredictionTime)
{
    if (!TrackedPlayer) {
        return FVector::ZeroVector;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // Use cached prediction if still valid
    if (CurrentTime - PredictionCacheTime < PredictionCacheValidTime) {
        return CachedPlayerPositionPrediction;
    }

    FVector CurrentPos = TrackedPlayer->GetActorLocation();
    FVector CurrentVel = TrackedPlayer->GetVelocity();

    // Basic linear prediction
    FVector BasicPrediction = CurrentPos + (CurrentVel * PredictionTime);

    // Enhanced prediction based on difficulty
    if (CurrentSettings.PredictionAccuracy > 0.5f && CurrentPlayerPattern.RecentPositions.Num() > 3) {
        // Acceleration-based prediction
        FVector LastVel = (CurrentPlayerPattern.RecentPositions.Last() - CurrentPlayerPattern.RecentPositions[CurrentPlayerPattern.RecentPositions.Num() - 2]) / (1.0f / 60.0f); // Assume 60 FPS

        FVector Acceleration = (CurrentVel - LastVel) / (1.0f / 60.0f);

        // Kinematic prediction: pos = pos0 + vel*t + 0.5*acc*t^2
        CachedPlayerPositionPrediction = CurrentPos + (CurrentVel * PredictionTime) + (0.5f * Acceleration * PredictionTime * PredictionTime);
    } else {
        CachedPlayerPositionPrediction = BasicPrediction;
    }

    // Pattern-based trajectory modification
    if (CurrentSettings.bCanCounterAdapt && CurrentPlayerPattern.bPrefersCircleStrafing) {
        // Predict circular movement
        float AngularVelocity = 2.0f; // Radians per second - could be learned
        FVector ToPlayer = CurrentPos - OwnerPawn->GetActorLocation();
        float CurrentAngle = FMath::Atan2(ToPlayer.Y, ToPlayer.X);
        float PredictedAngle = CurrentAngle + (AngularVelocity * PredictionTime);

        float Distance = ToPlayer.Size();
        CachedPlayerPositionPrediction = OwnerPawn->GetActorLocation() + FVector(FMath::Cos(PredictedAngle), FMath::Sin(PredictedAngle), 0.0f) * Distance;
    }

    PredictionCacheTime = CurrentTime;

    return CachedPlayerPositionPrediction;
}

FGameplayTag UEliteAIIntelligenceComponent::GetOptimalAttackAction(APawn* Target)
{
    if (!OwnerAIController) {
        return FGameplayTag();
    }

    // Get ACF Combat Behavior Component for action selection
    if (UACFCombatBehaviourComponent* CombatComp = OwnerAIController->GetCombatBehaviorComponent()) {
        float DistanceToTarget = FVector::Dist(OwnerPawn->GetActorLocation(), Target->GetActorLocation());

        // Use ACF's intelligent combat state selection
        EAICombatState OptimalState = CombatComp->GetBestCombatStateByTargetDistance(DistanceToTarget);

        // Enhanced action selection based on elite intelligence
        if (CurrentSettings.TacticalPlanningDepth > 0.5f) {
            // Consider player's recent actions to select counter-actions
            if (CurrentPlayerPattern.AttackTimings.Num() > 0) {
                float TimeSincePlayerAttack = GetWorld()->GetTimeSeconds() - CurrentPlayerPattern.AttackTimings.Last();

                // Use different actions based on timing
                if (TimeSincePlayerAttack < 0.5f) {
                    // Player just attacked - use defensive action
                    return FGameplayTag::RequestGameplayTag(FName("Action.DefensiveStrike"));
                } else {
                    // Player hasn't attacked recently - use aggressive action
                    return FGameplayTag::RequestGameplayTag(FName("Action.AggressiveStrike"));
                }
            }
        }

        // Fallback to standard ACF action
        return FGameplayTag::RequestGameplayTag(FName("Action.DefaultAttack"));
    }

    return FGameplayTag::RequestGameplayTag(FName("Action.DefaultAttack"));
}

// Tactical Intelligence
FVector UEliteAIIntelligenceComponent::GetOptimalFlankingPosition(APawn* Target)
{
    if (!Target || !OwnerPawn) {
        return FVector::ZeroVector;
    }

    FVector TargetPos = Target->GetActorLocation();
    FVector MyPos = OwnerPawn->GetActorLocation();
    FVector ToTarget = (TargetPos - MyPos).GetSafeNormal();

    // Basic flanking - 90 degrees to side
    FVector RightVector = FVector::CrossProduct(ToTarget, FVector::UpVector);

    // Advanced flanking based on intelligence level
    if (CurrentSettings.FlankingIntelligence > 0.7f) {
        // Predictive flanking - flank where player will be
        FVector PredictedTargetPos = PredictPlayerPosition(2.0f);
        FVector ToPredictedTarget = (PredictedTargetPos - MyPos).GetSafeNormal();
        RightVector = FVector::CrossProduct(ToPredictedTarget, FVector::UpVector);
    }

    // Choose optimal side based on environment and player patterns
    bool bFlankRight = true;

    if (CurrentPlayerPattern.PreferredDodgeDirection.Size() > 0.1f) {
        // Flank opposite to player's preferred dodge direction
        float DotProduct = FVector::DotProduct(RightVector, CurrentPlayerPattern.PreferredDodgeDirection);
        bFlankRight = DotProduct < 0.0f;
    }

    FVector FlankDirection = bFlankRight ? RightVector : -RightVector;
    float FlankDistance = FMath::Lerp(600.0f, 400.0f, CurrentSettings.FlankingIntelligence);

    FVector FlankPosition = TargetPos + (FlankDirection * FlankDistance);

    OnEliteBehaviorTriggered.Broadcast(TEXT("FlankingManeuver"), CurrentSettings.FlankingIntelligence);

    return FlankPosition;
}

bool UEliteAIIntelligenceComponent::ShouldExecuteTacticalRetreat()
{
    if (!OwnerPawn || CurrentSettings.TacticalPlanningDepth < 0.3f) {
        return false;
    }

    // Analyze tactical situation
    if (UACFDamageHandlerComponent* DamageHandler = OwnerPawn->FindComponentByClass<UACFDamageHandlerComponent>()) {
        float HealthPercentage = DamageHandler->GetCurrentHealth() / DamageHandler->GetMaxHealth();

        // Intelligent retreat thresholds based on difficulty
        float RetreatThreshold = FMath::Lerp(0.2f, 0.6f, CurrentSettings.TacticalPlanningDepth);

        if (HealthPercentage < RetreatThreshold) {
            // Consider tactical advantages of retreating
            if (CurrentSettings.bCanManipulatePlayer) {
                // Tactical retreat to bait player into disadvantageous position
                OnEliteBehaviorTriggered.Broadcast(TEXT("TacticalRetreat"), 1.0f);
                return true;
            }

            return true;
        }
    }

    // Retreat if outnumbered (future enhancement for multiplayer)
    // Retreat if low on resources (ammunition, etc.)

    return false;
}

FEliteTacticalPlan UEliteAIIntelligenceComponent::GenerateTacticalPlan(APawn* Target)
{
    FEliteTacticalPlan NewPlan;

    if (!Target || !OwnerPawn || CurrentSettings.TacticalPlanningDepth < 0.1f) {
        return NewPlan;
    }

    NewPlan.PredictedPlayerPosition = PredictPlayerPosition(3.0f);
    NewPlan.PlanConfidence = CurrentSettings.TacticalPlanningDepth;
    NewPlan.ExecutionStartTime = GetWorld()->GetTimeSeconds();

    // Generate tactical waypoints
    FVector CurrentPos = OwnerPawn->GetActorLocation();
    FVector TargetPos = Target->GetActorLocation();

    // Multi-step tactical plan based on difficulty
    int32 PlanSteps = FMath::RoundToInt(CurrentSettings.TacticalPlanningDepth * 5.0f) + 1;

    for (int32 i = 0; i < PlanSteps; ++i) {
        FVector StepPosition;
        FGameplayTag StepAction;
        float StepTiming = i * 2.0f; // 2 seconds per step

        switch (i) {
        case 0:
            // Step 1: Move to flanking position
            StepPosition = GetOptimalFlankingPosition(Target);
            StepAction = FGameplayTag::RequestGameplayTag(FName("Action.Move"));
            break;

        case 1:
            // Step 2: Attack from flank
            StepPosition = NewPlan.PredictedPlayerPosition;
            StepAction = GetOptimalAttackAction(Target);
            break;

        case 2:
            // Step 3: Reposition based on predicted player response
            if (CurrentPlayerPattern.bPrefersCircleStrafing) {
                // Counter circle-strafing
                FVector CounterPos = CurrentPos + FVector(300, 0, 0);
                StepPosition = CounterPos;
            } else {
                StepPosition = GetOptimalFlankingPosition(Target);
            }
            StepAction = FGameplayTag::RequestGameplayTag(FName("Action.Reposition"));
            break;

        default:
            // Advanced steps for highest difficulties
            if (CurrentSettings.bCanManipulatePlayer) {
                // Psychological manipulation step
                StepPosition = GetPlayerBaitPosition();
                StepAction = FGameplayTag::RequestGameplayTag(FName("Action.Bait"));
            } else {
                StepPosition = GetOptimalFlankingPosition(Target);
                StepAction = GetOptimalAttackAction(Target);
            }
            break;
        }

        NewPlan.PlannedPositions.Add(StepPosition);
        NewPlan.PlannedActions.Add(StepAction);
        NewPlan.ActionTimings.Add(StepTiming);
    }

    OnTacticalPlanExecuted.Broadcast(NewPlan);

    return NewPlan;
}

void UEliteAIIntelligenceComponent::ExecuteTacticalPlan(const FEliteTacticalPlan& Plan)
{
    if (!OwnerAIController || !Plan.bIsExecuting || Plan.PlannedPositions.Num() == 0) {
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float ExecutionTime = CurrentTime - Plan.ExecutionStartTime;

    // Find current step in plan
    int32 CurrentStep = 0;
    for (int32 i = 0; i < Plan.ActionTimings.Num(); ++i) {
        if (ExecutionTime >= Plan.ActionTimings[i]) {
            CurrentStep = i;
        }
    }

    if (CurrentStep < Plan.PlannedPositions.Num()) {
        // Execute current step
        FVector TargetPosition = Plan.PlannedPositions[CurrentStep];
        FGameplayTag TargetAction = Plan.PlannedActions[CurrentStep];

        // Move to position
        if (OwnerAIController->GetBlackboardComponent()) {
            OwnerAIController->SetTargetLocationBK(TargetPosition);
        }

        // Execute action
        if (UACFActionsManagerComponent* ActionsManager = OwnerPawn->FindComponentByClass<UACFActionsManagerComponent>()) {
            if (TargetAction.IsValid() && ActionsManager->CanExecuteAction(TargetAction)) {
                ActionsManager->TriggerAction(TargetAction, EActionPriority::EHigh);
            }
        }
    }

    // Mark plan as complete if all steps executed
    if (CurrentStep >= Plan.PlannedPositions.Num() - 1) {
        CurrentTacticalPlan.bIsExecuting = false;
    }
}

// Learning and Adaptation
void UEliteAIIntelligenceComponent::RecordPlayerAction(APawn* Player, const FVector& ActionPosition, const FString& ActionType)
{
    if (!Player || CurrentSettings.PatternLearningSpeed <= 0.0f) {
        return;
    }

    ProcessPlayerCombatAction(Player, ActionPosition, ActionType);

    // Trigger immediate adaptation for highest difficulties
    if (CurrentSettings.bCanCounterAdapt) {
        AdaptToPlayerBehavior();
    }
}

void UEliteAIIntelligenceComponent::AnalyzePlayerPatterns()
{
    if (!TrackedPlayer || CurrentPlayerPattern.RecentPositions.Num() < 5) {
        return;
    }

    // Analyze movement patterns
    float TotalDistance = 0.0f;
    FVector TotalDodgeDirection = FVector::ZeroVector;

    for (int32 i = 1; i < CurrentPlayerPattern.RecentPositions.Num(); ++i) {
        FVector Movement = CurrentPlayerPattern.RecentPositions[i] - CurrentPlayerPattern.RecentPositions[i - 1];
        TotalDistance += Movement.Size();

        if (Movement.Size() > 100.0f) // Significant movement
        {
            TotalDodgeDirection += Movement.GetSafeNormal();
        }
    }

    // Calculate averages
    CurrentPlayerPattern.AverageMovementSpeed = TotalDistance / CurrentPlayerPattern.RecentPositions.Num();
    CurrentPlayerPattern.PreferredDodgeDirection = TotalDodgeDirection.GetSafeNormal();

    // Analyze attack patterns
    if (CurrentPlayerPattern.AttackTimings.Num() > 1) {
        float TotalTime = CurrentPlayerPattern.AttackTimings.Last() - CurrentPlayerPattern.AttackTimings[0];
        CurrentPlayerPattern.AttackFrequency = TotalTime / CurrentPlayerPattern.AttackTimings.Num();
    }

    // Detect circle strafing
    int32 CircularMoves = 0;
    if (CurrentPlayerPattern.RecentPositions.Num() > 4) {
        for (int32 i = 2; i < CurrentPlayerPattern.RecentPositions.Num() - 2; ++i) {
            FVector Prev = CurrentPlayerPattern.RecentPositions[i - 1] - CurrentPlayerPattern.RecentPositions[i - 2];
            FVector Curr = CurrentPlayerPattern.RecentPositions[i] - CurrentPlayerPattern.RecentPositions[i - 1];
            FVector Next = CurrentPlayerPattern.RecentPositions[i + 1] - CurrentPlayerPattern.RecentPositions[i];

            float CrossProd1 = FVector::CrossProduct(Prev, Curr).Z;
            float CrossProd2 = FVector::CrossProduct(Curr, Next).Z;

            if (FMath::Sign(CrossProd1) == FMath::Sign(CrossProd2) && FMath::Abs(CrossProd1) > 100.0f) {
                CircularMoves++;
            }
        }
    }

    CurrentPlayerPattern.bPrefersCircleStrafing = CircularMoves > (CurrentPlayerPattern.RecentPositions.Num() * 0.6f);

    // Calculate pattern confidence
    CurrentPlayerPattern.PatternConfidence = FMath::Min(1.0f,
        (CurrentPlayerPattern.RecentPositions.Num() / 20.0f) * CurrentSettings.PatternLearningSpeed);

    OnPlayerPatternDetected.Broadcast(CurrentPlayerPattern);
}

void UEliteAIIntelligenceComponent::AdaptToPlayerBehavior()
{
    if (CurrentPlayerPattern.PatternConfidence < 0.3f) {
        return;
    }

    // Adapt movement patterns
    if (CurrentPlayerPattern.bPrefersCircleStrafing) {
        // Develop counter-strafing behavior
        OnEliteBehaviorTriggered.Broadcast(TEXT("CounterStrafing"), CurrentPlayerPattern.PatternConfidence);
    }

    // Adapt attack timing
    if (CurrentPlayerPattern.AttackFrequency > 0.0f) {
        // Adjust our attack timing to counter player rhythm
        OnEliteBehaviorTriggered.Broadcast(TEXT("CounterRhythm"), CurrentPlayerPattern.PatternConfidence);
    }

    // Store historical patterns for long-term learning
    if (HistoricalPatterns.Num() >= 10) {
        HistoricalPatterns.RemoveAt(0);
    }
    HistoricalPatterns.Add(CurrentPlayerPattern);
}

// Psychological Warfare
void UEliteAIIntelligenceComponent::ExecutePsychologicalTactic(const FString& TacticName)
{
    if (!CurrentSettings.bCanManipulatePlayer) {
        return;
    }

    if (TacticName == "FakeRetreat") {
        // Execute fake retreat to bait player
        if (OwnerAIController) {
            FVector BaitPosition = GetPlayerBaitPosition();
            OwnerAIController->SetTargetLocationBK(BaitPosition);
        }
    } else if (TacticName == "FeintAttack") {
        // Start attack animation then cancel into different action
        if (UACFActionsManagerComponent* ActionsManager = OwnerPawn->FindComponentByClass<UACFActionsManagerComponent>()) {
            // This would require custom action system support
            OnEliteBehaviorTriggered.Broadcast(TEXT("FeintAttack"), 1.0f);
        }
    } else if (TacticName == "RhythmBreak") {
        // Deliberately break established combat rhythm
        OnEliteBehaviorTriggered.Broadcast(TEXT("RhythmBreak"), 1.0f);
    }
}

bool UEliteAIIntelligenceComponent::ShouldBaitPlayer()
{
    return CurrentSettings.bCanManipulatePlayer && CurrentPlayerPattern.PatternConfidence > 0.5f && FMath::RandRange(0.0f, 1.0f) < 0.3f; // 30% chance when conditions are met
}

FVector UEliteAIIntelligenceComponent::GetPlayerBaitPosition()
{
    if (!TrackedPlayer || !OwnerPawn) {
        return FVector::ZeroVector;
    }

    // Position that appears advantageous to player but is actually a trap
    FVector PlayerPos = TrackedPlayer->GetActorLocation();
    FVector MyPos = OwnerPawn->GetActorLocation();
    FVector ToPlayer = (PlayerPos - MyPos).GetSafeNormal();

    // Position that looks like retreat but sets up counter-attack
    FVector BaitPosition = MyPos - (ToPlayer * 800.0f);

    return BaitPosition;
}

// Private Methods
void UEliteAIIntelligenceComponent::InitializeDifficultySettings()
{
    // Define presets for each difficulty level

    // Level 1: Novice
    FEliteDifficultySettings Novice;
    Novice.IntelligenceMode = EEliteIntelligenceMode::Reactive;
    Novice.ReactionTimeMultiplier = 0.9f;
    Novice.PredictionAccuracy = 0.1f;
    Novice.PatternLearningSpeed = 0.1f;
    Novice.TacticalPlanningDepth = 0.2f;
    Novice.DodgePerfection = 0.3f;
    Novice.AttackPrediction = 0.1f;
    Novice.FlankingIntelligence = 0.2f;
    Novice.EnvironmentUsage = 0.1f;
    DifficultyPresets.Add(EEliteDifficultyLevel::Novice, Novice);

    // Level 2: Skilled
    FEliteDifficultySettings Skilled;
    Skilled.IntelligenceMode = EEliteIntelligenceMode::Reactive;
    Skilled.ReactionTimeMultiplier = 0.8f;
    Skilled.PredictionAccuracy = 0.2f;
    Skilled.PatternLearningSpeed = 0.2f;
    Skilled.TacticalPlanningDepth = 0.3f;
    Skilled.DodgePerfection = 0.4f;
    Skilled.AttackPrediction = 0.2f;
    Skilled.FlankingIntelligence = 0.3f;
    Skilled.EnvironmentUsage = 0.2f;
    DifficultyPresets.Add(EEliteDifficultyLevel::Skilled, Skilled);

    // Level 3: Veteran
    FEliteDifficultySettings Veteran;
    Veteran.IntelligenceMode = EEliteIntelligenceMode::Predictive;
    Veteran.ReactionTimeMultiplier = 0.7f;
    Veteran.PredictionAccuracy = 0.4f;
    Veteran.PatternLearningSpeed = 0.3f;
    Veteran.TacticalPlanningDepth = 0.4f;
    Veteran.DodgePerfection = 0.5f;
    Veteran.AttackPrediction = 0.3f;
    Veteran.FlankingIntelligence = 0.4f;
    Veteran.EnvironmentUsage = 0.3f;
    DifficultyPresets.Add(EEliteDifficultyLevel::Veteran, Veteran);

    // Level 4: Expert
    FEliteDifficultySettings Expert;
    Expert.IntelligenceMode = EEliteIntelligenceMode::Predictive;
    Expert.ReactionTimeMultiplier = 0.6f;
    Expert.PredictionAccuracy = 0.5f;
    Expert.PatternLearningSpeed = 0.4f;
    Expert.TacticalPlanningDepth = 0.5f;
    Expert.DodgePerfection = 0.6f;
    Expert.AttackPrediction = 0.4f;
    Expert.FlankingIntelligence = 0.5f;
    Expert.EnvironmentUsage = 0.4f;
    DifficultyPresets.Add(EEliteDifficultyLevel::Expert, Expert);

    // Level 5: Master
    FEliteDifficultySettings Master;
    Master.IntelligenceMode = EEliteIntelligenceMode::Adaptive;
    Master.ReactionTimeMultiplier = 0.5f;
    Master.PredictionAccuracy = 0.6f;
    Master.PatternLearningSpeed = 0.5f;
    Master.TacticalPlanningDepth = 0.6f;
    Master.DodgePerfection = 0.7f;
    Master.AttackPrediction = 0.5f;
    Master.FlankingIntelligence = 0.6f;
    Master.EnvironmentUsage = 0.5f;
    Master.bCanCounterAdapt = true;
    DifficultyPresets.Add(EEliteDifficultyLevel::Master, Master);

    // Level 6: Grandmaster
    FEliteDifficultySettings Grandmaster;
    Grandmaster.IntelligenceMode = EEliteIntelligenceMode::Adaptive;
    Grandmaster.ReactionTimeMultiplier = 0.4f;
    Grandmaster.PredictionAccuracy = 0.7f;
    Grandmaster.PatternLearningSpeed = 0.6f;
    Grandmaster.TacticalPlanningDepth = 0.7f;
    Grandmaster.DodgePerfection = 0.8f;
    Grandmaster.AttackPrediction = 0.6f;
    Grandmaster.FlankingIntelligence = 0.7f;
    Grandmaster.EnvironmentUsage = 0.6f;
    Grandmaster.bCanCounterAdapt = true;
    DifficultyPresets.Add(EEliteDifficultyLevel::Grandmaster, Grandmaster);

    // Level 7: Legend
    FEliteDifficultySettings Legend;
    Legend.IntelligenceMode = EEliteIntelligenceMode::Strategic;
    Legend.ReactionTimeMultiplier = 0.3f;
    Legend.PredictionAccuracy = 0.8f;
    Legend.PatternLearningSpeed = 0.7f;
    Legend.TacticalPlanningDepth = 0.8f;
    Legend.DodgePerfection = 0.85f;
    Legend.AttackPrediction = 0.7f;
    Legend.FlankingIntelligence = 0.8f;
    Legend.EnvironmentUsage = 0.7f;
    Legend.bCanCounterAdapt = true;
    Legend.bUsesFramePerfectTiming = true;
    DifficultyPresets.Add(EEliteDifficultyLevel::Legend, Legend);

    // Level 8: Nightmare
    FEliteDifficultySettings Nightmare;
    Nightmare.IntelligenceMode = EEliteIntelligenceMode::Strategic;
    Nightmare.ReactionTimeMultiplier = 0.2f;
    Nightmare.PredictionAccuracy = 0.9f;
    Nightmare.PatternLearningSpeed = 0.8f;
    Nightmare.TacticalPlanningDepth = 0.9f;
    Nightmare.DodgePerfection = 0.9f;
    Nightmare.AttackPrediction = 0.8f;
    Nightmare.FlankingIntelligence = 0.9f;
    Nightmare.EnvironmentUsage = 0.8f;
    Nightmare.bCanCounterAdapt = true;
    Nightmare.bUsesFramePerfectTiming = true;
    DifficultyPresets.Add(EEliteDifficultyLevel::Nightmare, Nightmare);

    // Level 9: Impossible
    FEliteDifficultySettings Impossible;
    Impossible.IntelligenceMode = EEliteIntelligenceMode::Psychological;
    Impossible.ReactionTimeMultiplier = 0.15f;
    Impossible.PredictionAccuracy = 0.95f;
    Impossible.PatternLearningSpeed = 0.9f;
    Impossible.TacticalPlanningDepth = 0.95f;
    Impossible.DodgePerfection = 0.95f;
    Impossible.AttackPrediction = 0.9f;
    Impossible.FlankingIntelligence = 0.95f;
    Impossible.EnvironmentUsage = 0.9f;
    Impossible.bCanCounterAdapt = true;
    Impossible.bUsesFramePerfectTiming = true;
    Impossible.bCanManipulatePlayer = true;
    DifficultyPresets.Add(EEliteDifficultyLevel::Impossible, Impossible);

    // Level 10: Godlike
    FEliteDifficultySettings Godlike;
    Godlike.IntelligenceMode = EEliteIntelligenceMode::Psychological;
    Godlike.ReactionTimeMultiplier = 0.1f;
    Godlike.PredictionAccuracy = 1.0f;
    Godlike.PatternLearningSpeed = 1.0f;
    Godlike.TacticalPlanningDepth = 1.0f;
    Godlike.DodgePerfection = 0.98f;
    Godlike.AttackPrediction = 0.95f;
    Godlike.FlankingIntelligence = 1.0f;
    Godlike.EnvironmentUsage = 1.0f;
    Godlike.bCanCounterAdapt = true;
    Godlike.bUsesFramePerfectTiming = true;
    Godlike.bCanManipulatePlayer = true;
    DifficultyPresets.Add(EEliteDifficultyLevel::Godlike, Godlike);
}

void UEliteAIIntelligenceComponent::UpdateDifficultySettings()
{
    if (DifficultyPresets.Contains(CurrentDifficulty)) {
        CurrentSettings = DifficultyPresets[CurrentDifficulty];
    }
}

void UEliteAIIntelligenceComponent::UpdatePlayerTracking()
{
    if (!TrackedPlayer) {
        TrackedPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        return;
    }

    ProcessPlayerMovement(TrackedPlayer);
}

void UEliteAIIntelligenceComponent::UpdateCombatState()
{
    bool bNewCombatState = false;

    if (OwnerAIController && TrackedPlayer) {
        float DistanceToPlayer = FVector::Dist(OwnerPawn->GetActorLocation(), TrackedPlayer->GetActorLocation());
        bNewCombatState = DistanceToPlayer < 2000.0f; // Consider combat range
    }

    if (bNewCombatState && !bInCombat) {
        bInCombat = true;
        CombatStartTime = GetWorld()->GetTimeSeconds();

        // Reset pattern learning for new combat encounter
        CurrentPlayerPattern = FPlayerBehaviorPattern();
    } else if (!bNewCombatState && bInCombat) {
        bInCombat = false;

        // Store learned patterns
        if (CurrentPlayerPattern.PatternConfidence > 0.3f) {
            AdaptToPlayerBehavior();
        }
    }
}

void UEliteAIIntelligenceComponent::UpdateFrameTiming(float DeltaTime)
{
    float FrameTimeMs = DeltaTime * 1000.0f;
    RecentFrameTimes.Add(FrameTimeMs);

    if (RecentFrameTimes.Num() > 60) {
        RecentFrameTimes.RemoveAt(0);
    }

    // Calculate average
    float TotalTime = 0.0f;
    for (float Time : RecentFrameTimes) {
        TotalTime += Time;
    }
    AverageFrameTime = RecentFrameTimes.Num() > 0 ? TotalTime / RecentFrameTimes.Num() : 16.67f;
}

FEliteDifficultySettings UEliteAIIntelligenceComponent::GetSettingsForDifficulty(EEliteDifficultyLevel Difficulty) const
{
    if (DifficultyPresets.Contains(Difficulty)) {
        return DifficultyPresets[Difficulty];
    }
    return FEliteDifficultySettings();
}

void UEliteAIIntelligenceComponent::ProcessPlayerMovement(APawn* Player)
{
    if (!Player || CurrentSettings.PatternLearningSpeed <= 0.0f) {
        return;
    }

    FVector CurrentPos = Player->GetActorLocation();
    CurrentPlayerPattern.RecentPositions.Add(CurrentPos);

    // Keep only recent positions
    if (CurrentPlayerPattern.RecentPositions.Num() > 50) {
        CurrentPlayerPattern.RecentPositions.RemoveAt(0);
    }
}

void UEliteAIIntelligenceComponent::ProcessPlayerCombatAction(APawn* Player, const FVector& ActionPosition, const FString& ActionType)
{
    if (!Player) {
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();

    if (ActionType == "Attack") {
        CurrentPlayerPattern.AttackPositions.Add(ActionPosition);
        CurrentPlayerPattern.AttackTimings.Add(CurrentTime);

        // Keep only recent attacks
        if (CurrentPlayerPattern.AttackPositions.Num() > 20) {
            CurrentPlayerPattern.AttackPositions.RemoveAt(0);
            CurrentPlayerPattern.AttackTimings.RemoveAt(0);
        }
    } else if (ActionType == "Dodge") {
        if (OwnerPawn) {
            FVector DodgeDirection = (ActionPosition - OwnerPawn->GetActorLocation()).GetSafeNormal();
            CurrentPlayerPattern.DodgeDirections.Add(DodgeDirection);

            if (CurrentPlayerPattern.DodgeDirections.Num() > 30) {
                CurrentPlayerPattern.DodgeDirections.RemoveAt(0);
            }
        }
    }
}

bool UEliteAIIntelligenceComponent::CanPredictWithAccuracy(float RequiredAccuracy) const
{
    return CurrentSettings.PredictionAccuracy >= RequiredAccuracy;
}

bool UEliteAIIntelligenceComponent::IsFramePerfectTimingRequired() const
{
    return CurrentSettings.bUsesFramePerfectTiming;
}

float UEliteAIIntelligenceComponent::GetCurrentReactionTime() const
{
    float BaseReactionTime = 0.5f; // Human baseline
    return BaseReactionTime * CurrentSettings.ReactionTimeMultiplier;
}

FVector UEliteAIIntelligenceComponent::CalculateAdvancedDodge(const FVector& ThreatDirection, const FVector& ThreatVelocity)
{
    // Advanced physics-based dodge calculation
    FVector PerpendicularDirection = FVector::CrossProduct(ThreatDirection, FVector::UpVector);

    // Consider threat velocity for optimal perpendicular dodge
    if (ThreatVelocity.Size() > 0.1f) {
        FVector ThreatVelNormalized = ThreatVelocity.GetSafeNormal();
        float VelocityAlignment = FVector::DotProduct(ThreatDirection, ThreatVelNormalized);

        // Adjust dodge based on threat velocity alignment
        PerpendicularDirection = PerpendicularDirection * (1.0f + VelocityAlignment);
    }

    return PerpendicularDirection.GetSafeNormal();
}

FVector UEliteAIIntelligenceComponent::CalculateInterceptPosition(APawn* Target, float ProjectileSpeed)
{
    if (!Target || !OwnerPawn) {
        return FVector::ZeroVector;
    }

    FVector TargetPos = Target->GetActorLocation();
    FVector TargetVel = Target->GetVelocity();
    FVector ShooterPos = OwnerPawn->GetActorLocation();

    // Solve intercept equation: |ProjectilePos(t) - TargetPos(t)| = 0
    float A = TargetVel.SizeSquared() - (ProjectileSpeed * ProjectileSpeed);
    FVector ToTarget = TargetPos - ShooterPos;
    float B = 2.0f * FVector::DotProduct(TargetVel, ToTarget);
    float C = ToTarget.SizeSquared();

    float Discriminant = (B * B) - (4.0f * A * C);

    if (Discriminant >= 0.0f && FMath::Abs(A) > KINDA_SMALL_NUMBER) {
        float T1 = (-B + FMath::Sqrt(Discriminant)) / (2.0f * A);
        float T2 = (-B - FMath::Sqrt(Discriminant)) / (2.0f * A);

        float InterceptTime = (T1 > 0.0f && T1 < T2) ? T1 : T2;
        if (InterceptTime > 0.0f) {
            return TargetPos + (TargetVel * InterceptTime);
        }
    }

    // Fallback to simple prediction
    return PredictPlayerPosition(1.0f);
}

FVector UEliteAIIntelligenceComponent::CalculateFlankingRoute(APawn* Target, float OptimalDistance)
{
    // Calculate multi-waypoint flanking route
    // This is a simplified version - could be expanded with proper pathfinding
    return GetOptimalFlankingPosition(Target);
}

void UEliteAIIntelligenceComponent::ExecuteCounterAdaptation()
{
    if (!CurrentSettings.bCanCounterAdapt || CurrentPlayerPattern.PatternConfidence < 0.5f) {
        return;
    }

    // Deliberately change behavior to counter player adaptations
    if (HistoricalPatterns.Num() > 2) {
        // Compare current pattern to historical patterns
        FPlayerBehaviorPattern& LastPattern = HistoricalPatterns.Last();

        // If player is adapting to our behavior, counter-adapt
        if (FMath::Abs(CurrentPlayerPattern.AverageMovementSpeed - LastPattern.AverageMovementSpeed) > 100.0f) {
            OnEliteBehaviorTriggered.Broadcast(TEXT("CounterAdaptation"), 1.0f);
        }
    }
}

void UEliteAIIntelligenceComponent::UpdatePsychologicalProfile()
{
    // Build psychological profile of player for manipulation tactics
    // This could track stress indicators, reaction patterns, etc.
}

bool UEliteAIIntelligenceComponent::ShouldUsePsychologicalWarfare() const
{
    return CurrentSettings.bCanManipulatePlayer && bInCombat && GetWorld()->GetTimeSeconds() - CombatStartTime > 5.0f;
}