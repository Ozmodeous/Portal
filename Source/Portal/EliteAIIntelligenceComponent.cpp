// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "EliteAIIntelligenceComponent.h"
#include "ACFAIController.h"
#include "Components/ACFCombatBehaviourComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UEliteAIIntelligenceComponent::UEliteAIIntelligenceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.2f; // 5 FPS for elite intelligence

    bEliteModeEnabled = false;
    CurrentDifficulty = EEliteDifficultyLevel::Novice;
    RecentFrameTimes.Reserve(30);
}

void UEliteAIIntelligenceComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerAIController = Cast<AACFAIController>(GetOwner());
    UpdateDifficultySettings();

    UE_LOG(LogTemp, Log, TEXT("Elite AI Intelligence initialized for %s"),
        OwnerAIController ? *OwnerAIController->GetName() : TEXT("Unknown"));
}

void UEliteAIIntelligenceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEliteModeEnabled || !OwnerAIController)
        return;

    UpdateFrameTiming(DeltaTime);
    UpdatePlayerTracking();

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAnalysisTime > 2.0f) {
        AnalyzePlayerBehavior();
        LastAnalysisTime = CurrentTime;
    }
}

void UEliteAIIntelligenceComponent::SetEliteMode(bool bEnabled)
{
    bEliteModeEnabled = bEnabled;
    if (bEnabled) {
        UpdateDifficultySettings();
        UE_LOG(LogTemp, Warning, TEXT("Elite AI Mode ENABLED for %s"),
            OwnerAIController ? *OwnerAIController->GetName() : TEXT("Unknown"));
    }
}

void UEliteAIIntelligenceComponent::SetDifficultyLevel(EEliteDifficultyLevel NewDifficulty)
{
    CurrentDifficulty = NewDifficulty;
    UpdateDifficultySettings();
}

void UEliteAIIntelligenceComponent::RecordPlayerAction(APawn* Player, const FVector& ActionLocation, const FString& ActionType)
{
    if (!Player || !bEliteModeEnabled)
        return;

    TrackedPlayer = Player;
    float CurrentTime = GetWorld()->GetTimeSeconds();

    // Record position
    CurrentPlayerPattern.RecentPositions.Add(ActionLocation);
    if (CurrentPlayerPattern.RecentPositions.Num() > 15) {
        CurrentPlayerPattern.RecentPositions.RemoveAt(0);
    }

    // Record attack timing
    if (ActionType.Contains("Attack") || ActionType.Contains("Fire")) {
        CurrentPlayerPattern.AttackTimings.Add(CurrentTime);
        if (CurrentPlayerPattern.AttackTimings.Num() > 8) {
            CurrentPlayerPattern.AttackTimings.RemoveAt(0);
        }
    }

    // Record dodge direction
    if (ActionType.Contains("Dodge") || ActionType.Contains("Evade")) {
        if (OwnerAIController && OwnerAIController->GetPawn()) {
            FVector DodgeDir = (ActionLocation - OwnerAIController->GetPawn()->GetActorLocation()).GetSafeNormal();
            CurrentPlayerPattern.DodgeDirections.Add(DodgeDir);
            if (CurrentPlayerPattern.DodgeDirections.Num() > 6) {
                CurrentPlayerPattern.DodgeDirections.RemoveAt(0);
            }
        }
    }
}

EAICombatState UEliteAIIntelligenceComponent::GetOptimalCombatState(APawn* Target)
{
    if (!Target || !OwnerAIController)
        return EAICombatState::EMeleeCombat;

    // Try to find ACF Combat Behavior Component on the AI's pawn
    UACFCombatBehaviourComponent* CombatComp = nullptr;
    if (APawn* AIPawn = OwnerAIController->GetPawn()) {
        CombatComp = AIPawn->FindComponentByClass<UACFCombatBehaviourComponent>();
    }

    if (!CombatComp)
        return EAICombatState::EMeleeCombat;

    float DistanceToTarget = FVector::Dist(OwnerAIController->GetPawn()->GetActorLocation(), Target->GetActorLocation());

    // Use ACF's built-in distance-based state selection
    EAICombatState OptimalState = CombatComp->GetBestCombatStateByTargetDistance(DistanceToTarget);

    // Elite AI enhancement: Consider player patterns
    if (CurrentSettings.FlankingIntelligence > 0.5f && CurrentPlayerPattern.bPrefersCircleStrafing) {
        // Player circle strafes, use ranged combat
        return EAICombatState::ERangedCombat;
    }

    if (CurrentSettings.PredictionAccuracy > 0.7f && CurrentPlayerPattern.PredictabilityScore > 0.8f) {
        // Predictable player, use aggressive melee
        return EAICombatState::EMeleeCombat;
    }

    return OptimalState;
}

FGameplayTag UEliteAIIntelligenceComponent::GetOptimalACFAction(EAICombatState CombatState)
{
    if (!OwnerAIController)
        return FGameplayTag();

    // Try to find ACF Combat Behavior Component
    UACFCombatBehaviourComponent* CombatComp = nullptr;
    if (APawn* AIPawn = OwnerAIController->GetPawn()) {
        CombatComp = AIPawn->FindComponentByClass<UACFCombatBehaviourComponent>();
    }

    if (!CombatComp)
        return FGameplayTag();

    // Elite AI enhancement: Modify timing based on player patterns
    if (CurrentSettings.bCanCounterAdapt && CurrentPlayerPattern.AttackTimings.Num() > 2) {
        float TimeSinceLastPlayerAttack = GetWorld()->GetTimeSeconds() - (CurrentPlayerPattern.AttackTimings.Num() > 0 ? CurrentPlayerPattern.AttackTimings.Last() : 0.0f);

        // Counter-attack timing based on player patterns
        if (TimeSinceLastPlayerAttack < 0.3f) {
            // Player just attacked, use defensive actions
            OnEliteBehaviorTriggered.Broadcast("DefensiveCounter", CurrentSettings.PredictionAccuracy);
        }
    }

    // Return empty tag to let ACF handle standard action selection
    return FGameplayTag();
}

bool UEliteAIIntelligenceComponent::ShouldDodgeNow(const FVector& ThreatDirection, float ThreatSpeed)
{
    if (!bEliteModeEnabled || CurrentSettings.DodgePerfection < 0.1f)
        return false;

    // Frame-perfect timing for highest difficulties
    if (CurrentSettings.bUsesFramePerfectTiming) {
        float ThreatDistance = ThreatDirection.Size();
        float TimeToImpact = ThreatDistance / ThreatSpeed;
        float ReactionTime = 0.25f * CurrentSettings.ReactionTimeMultiplier;
        return FMath::Abs(TimeToImpact - ReactionTime) < (AverageFrameTime * 0.001f);
    }

    return FMath::RandRange(0.0f, 1.0f) < CurrentSettings.DodgePerfection;
}

FVector UEliteAIIntelligenceComponent::PredictPlayerPosition(float PredictionTime)
{
    if (!TrackedPlayer || CurrentPlayerPattern.RecentPositions.Num() < 2) {
        return TrackedPlayer ? TrackedPlayer->GetActorLocation() : FVector::ZeroVector;
    }

    FVector CurrentPos = TrackedPlayer->GetActorLocation();
    FVector CurrentVel = TrackedPlayer->GetVelocity();

    // Basic linear prediction
    FVector BasicPrediction = CurrentPos + (CurrentVel * PredictionTime);

    // Enhanced prediction for higher difficulties
    if (CurrentSettings.PredictionAccuracy > 0.5f && CurrentPlayerPattern.RecentPositions.Num() >= 3) {
        // Account for circle strafing
        if (CurrentPlayerPattern.bPrefersCircleStrafing && OwnerAIController && OwnerAIController->GetPawn()) {
            FVector ToAI = (OwnerAIController->GetPawn()->GetActorLocation() - CurrentPos).GetSafeNormal();
            FVector CircleDir = FVector::CrossProduct(ToAI, FVector::UpVector);
            float Distance = FVector::Dist(CurrentPos, OwnerAIController->GetPawn()->GetActorLocation());

            float PredictedAngle = CurrentPlayerPattern.AverageMovementSpeed * PredictionTime / Distance;
            return OwnerAIController->GetPawn()->GetActorLocation() + FVector(FMath::Cos(PredictedAngle), FMath::Sin(PredictedAngle), 0.0f) * Distance;
        }
    }

    return BasicPrediction;
}

bool UEliteAIIntelligenceComponent::ShouldCounterAttack()
{
    if (!bEliteModeEnabled || CurrentPlayerPattern.AttackTimings.Num() < 2)
        return false;

    // Analyze player attack patterns for counter opportunities
    float TimeSinceLastAttack = GetWorld()->GetTimeSeconds() - CurrentPlayerPattern.AttackTimings.Last();

    if (CurrentPlayerPattern.AttackTimings.Num() >= 3) {
        // Calculate average time between attacks
        float TotalInterval = 0.0f;
        for (int32 i = 1; i < CurrentPlayerPattern.AttackTimings.Num(); i++) {
            TotalInterval += CurrentPlayerPattern.AttackTimings[i] - CurrentPlayerPattern.AttackTimings[i - 1];
        }
        float AvgInterval = TotalInterval / (CurrentPlayerPattern.AttackTimings.Num() - 1);

        // Counter-attack during player's vulnerable window
        return TimeSinceLastAttack > (AvgInterval * 0.3f) && TimeSinceLastAttack < (AvgInterval * 0.7f);
    }

    return TimeSinceLastAttack > 0.5f && TimeSinceLastAttack < 2.0f;
}

void UEliteAIIntelligenceComponent::UpdateDifficultySettings()
{
    CurrentSettings = GetSettingsForDifficulty(CurrentDifficulty);
}

void UEliteAIIntelligenceComponent::AnalyzePlayerBehavior()
{
    if (!TrackedPlayer || CurrentPlayerPattern.RecentPositions.Num() < 3)
        return;

    // Calculate average movement speed
    float TotalSpeed = 0.0f;
    for (int32 i = 1; i < CurrentPlayerPattern.RecentPositions.Num(); i++) {
        float Distance = FVector::Dist(CurrentPlayerPattern.RecentPositions[i], CurrentPlayerPattern.RecentPositions[i - 1]);
        TotalSpeed += Distance;
    }
    CurrentPlayerPattern.AverageMovementSpeed = TotalSpeed / FMath::Max(1, CurrentPlayerPattern.RecentPositions.Num() - 1);

    // Calculate preferred engagement distance
    if (CurrentPlayerPattern.RecentPositions.Num() > 0 && OwnerAIController && OwnerAIController->GetPawn()) {
        float TotalDistance = 0.0f;
        for (const FVector& Pos : CurrentPlayerPattern.RecentPositions) {
            TotalDistance += FVector::Dist(Pos, OwnerAIController->GetPawn()->GetActorLocation());
        }
        CurrentPlayerPattern.PreferredEngagementDistance = TotalDistance / CurrentPlayerPattern.RecentPositions.Num();
    }

    // Calculate preferred dodge direction
    if (CurrentPlayerPattern.DodgeDirections.Num() > 1) {
        FVector AverageDodge = FVector::ZeroVector;
        for (const FVector& Dodge : CurrentPlayerPattern.DodgeDirections) {
            AverageDodge += Dodge;
        }
        CurrentPlayerPattern.PreferredDodgeDirection = AverageDodge.GetSafeNormal();
    }

    // Detect circle strafing
    CurrentPlayerPattern.bPrefersCircleStrafing = DetectCircleStrafePattern();

    // Calculate predictability
    CurrentPlayerPattern.PredictabilityScore = CalculatePredictabilityScore();
}

void UEliteAIIntelligenceComponent::UpdatePlayerTracking()
{
    if (!OwnerAIController)
        return;

    if (AActor* FocusActor = OwnerAIController->GetFocusActor()) {
        if (APawn* PlayerPawn = Cast<APawn>(FocusActor)) {
            if (PlayerPawn->IsPlayerControlled()) {
                TrackedPlayer = PlayerPawn;
            }
        }
    }
}

FEliteSettings UEliteAIIntelligenceComponent::GetSettingsForDifficulty(EEliteDifficultyLevel Difficulty) const
{
    FEliteSettings Settings;

    switch (Difficulty) {
    case EEliteDifficultyLevel::Disabled:
        break;

    case EEliteDifficultyLevel::Novice:
        Settings.ReactionTimeMultiplier = 1.2f;
        Settings.PredictionAccuracy = 0.1f;
        Settings.DodgePerfection = 0.2f;
        break;

    case EEliteDifficultyLevel::Skilled:
        Settings.ReactionTimeMultiplier = 1.0f;
        Settings.PredictionAccuracy = 0.3f;
        Settings.DodgePerfection = 0.4f;
        Settings.FlankingIntelligence = 0.3f;
        break;

    case EEliteDifficultyLevel::Veteran:
        Settings.ReactionTimeMultiplier = 0.8f;
        Settings.PredictionAccuracy = 0.5f;
        Settings.DodgePerfection = 0.6f;
        Settings.FlankingIntelligence = 0.5f;
        break;

    case EEliteDifficultyLevel::Expert:
        Settings.ReactionTimeMultiplier = 0.6f;
        Settings.PredictionAccuracy = 0.7f;
        Settings.DodgePerfection = 0.8f;
        Settings.FlankingIntelligence = 0.7f;
        Settings.bCanCounterAdapt = true;
        break;

    case EEliteDifficultyLevel::Master:
        Settings.ReactionTimeMultiplier = 0.4f;
        Settings.PredictionAccuracy = 0.85f;
        Settings.DodgePerfection = 0.9f;
        Settings.FlankingIntelligence = 0.9f;
        Settings.bCanCounterAdapt = true;
        break;

    case EEliteDifficultyLevel::Grandmaster:
        Settings.ReactionTimeMultiplier = 0.2f;
        Settings.PredictionAccuracy = 0.95f;
        Settings.DodgePerfection = 0.95f;
        Settings.FlankingIntelligence = 1.0f;
        Settings.bCanCounterAdapt = true;
        Settings.bUsesFramePerfectTiming = true;
        break;
    }

    return Settings;
}

bool UEliteAIIntelligenceComponent::DetectCircleStrafePattern() const
{
    if (CurrentPlayerPattern.RecentPositions.Num() < 5 || !OwnerAIController || !OwnerAIController->GetPawn())
        return false;

    FVector CenterPos = OwnerAIController->GetPawn()->GetActorLocation();
    float DistanceVariance = 0.0f;
    float AverageDistance = 0.0f;

    for (const FVector& Pos : CurrentPlayerPattern.RecentPositions) {
        AverageDistance += FVector::Dist(Pos, CenterPos);
    }
    AverageDistance /= CurrentPlayerPattern.RecentPositions.Num();

    for (const FVector& Pos : CurrentPlayerPattern.RecentPositions) {
        float Distance = FVector::Dist(Pos, CenterPos);
        DistanceVariance += FMath::Abs(Distance - AverageDistance);
    }
    DistanceVariance /= CurrentPlayerPattern.RecentPositions.Num();

    return DistanceVariance < 150.0f; // Low variance indicates circular movement
}

float UEliteAIIntelligenceComponent::CalculatePredictabilityScore() const
{
    if (CurrentPlayerPattern.RecentPositions.Num() < 3)
        return 0.5f;

    float MovementVariance = 0.0f;
    for (int32 i = 2; i < CurrentPlayerPattern.RecentPositions.Num(); i++) {
        FVector Move1 = CurrentPlayerPattern.RecentPositions[i - 1] - CurrentPlayerPattern.RecentPositions[i - 2];
        FVector Move2 = CurrentPlayerPattern.RecentPositions[i] - CurrentPlayerPattern.RecentPositions[i - 1];
        MovementVariance += FVector::Dist(Move1, Move2);
    }
    MovementVariance /= FMath::Max(1, CurrentPlayerPattern.RecentPositions.Num() - 2);

    return FMath::Clamp(1.0f - (MovementVariance / 1000.0f), 0.0f, 1.0f);
}

void UEliteAIIntelligenceComponent::UpdateFrameTiming(float DeltaTime)
{
    float FrameTimeMs = DeltaTime * 1000.0f;
    RecentFrameTimes.Add(FrameTimeMs);

    if (RecentFrameTimes.Num() > 30) {
        RecentFrameTimes.RemoveAt(0);
    }

    float TotalTime = 0.0f;
    for (float FrameTime : RecentFrameTimes) {
        TotalTime += FrameTime;
    }
    AverageFrameTime = RecentFrameTimes.Num() > 0 ? TotalTime / RecentFrameTimes.Num() : 16.67f;
}