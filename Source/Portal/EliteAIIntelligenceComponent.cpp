// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "EliteAIIntelligenceComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ACFActionsManagerComponent.h"
#include "Engine/World.h"
#include "Game/ACFTypes.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PortalDefenseAIController.h" // Include here instead of in header
#include "TimerManager.h"

UEliteAIIntelligenceComponent::UEliteAIIntelligenceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // 10 FPS for performance

    // Initialize default settings
    bEliteModeEnabled = false;
    CurrentDifficultyLevel = EEliteDifficultyLevel::Novice;
    CurrentIntelligenceMode = EEliteIntelligenceMode::Reactive;

    // Initialize tracking variables
    TrackedPlayer = nullptr;
    TrackingStartTime = 0.0f;
    bIsTrackingPlayer = false;

    // Initialize prediction data
    CachedPlayerPositionPrediction = FVector::ZeroVector;
    PredictionConfidence = 0.0f;
    LastPredictionTime = 0.0f;

    // Initialize adaptation data
    AdaptationCooldown = 0.0f;

    // Initialize references
    OwnerController = nullptr;
    OwnerPawn = nullptr;

    // Apply initial settings
    ApplyDifficultySettings();
}

void UEliteAIIntelligenceComponent::BeginPlay()
{
    Super::BeginPlay();

    // Get owner references
    OwnerController = Cast<APortalDefenseAIController>(GetOwner());
    if (OwnerController) {
        OwnerPawn = OwnerController->GetPawn();
    }

    // Initialize elite mode if enabled
    if (bEliteModeEnabled) {
        ApplyDifficultySettings();

        // Start player tracking if possible
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
            if (APawn* PlayerPawn = PC->GetPawn()) {
                StartPlayerTracking(PlayerPawn);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Elite AI Intelligence Component initialized for %s"),
        OwnerController ? *OwnerController->GetName() : TEXT("Unknown"));
}

void UEliteAIIntelligenceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEliteModeEnabled || !bIsTrackingPlayer) {
        return;
    }

    // Update player pattern analysis
    UpdatePlayerPattern();

    // Update predictions
    UpdatePredictions();

    // Update adaptation logic
    UpdateAdaptation();

    // Reduce adaptation cooldown
    if (AdaptationCooldown > 0.0f) {
        AdaptationCooldown -= DeltaTime;
    }
}

// Core Elite AI Functions
void UEliteAIIntelligenceComponent::SetEliteMode(bool bEnabled)
{
    bEliteModeEnabled = bEnabled;

    if (bEliteModeEnabled) {
        ApplyDifficultySettings();
        UE_LOG(LogTemp, Warning, TEXT("Elite AI Mode ENABLED for %s"),
            OwnerController ? *OwnerController->GetName() : TEXT("Unknown"));
    } else {
        StopPlayerTracking();
        ResetBehaviorPatterns();
        UE_LOG(LogTemp, Log, TEXT("Elite AI Mode disabled for %s"),
            OwnerController ? *OwnerController->GetName() : TEXT("Unknown"));
    }
}

void UEliteAIIntelligenceComponent::SetDifficultyLevel(EEliteDifficultyLevel NewLevel)
{
    CurrentDifficultyLevel = NewLevel;
    ApplyDifficultySettings();

    UE_LOG(LogTemp, Warning, TEXT("Elite AI Difficulty set to %d for %s"),
        (int32)NewLevel, OwnerController ? *OwnerController->GetName() : TEXT("Unknown"));
}

void UEliteAIIntelligenceComponent::SetIntelligenceMode(EEliteIntelligenceMode NewMode)
{
    CurrentIntelligenceMode = NewMode;
    CurrentSettings.bCanCounterAdapt = (NewMode == EEliteIntelligenceMode::Adaptive || NewMode == EEliteIntelligenceMode::Strategic || NewMode == EEliteIntelligenceMode::Psychological);
}

// Player Analysis Functions
void UEliteAIIntelligenceComponent::StartPlayerTracking(APawn* Player)
{
    if (!Player || !bEliteModeEnabled) {
        return;
    }

    TrackedPlayer = Player;
    bIsTrackingPlayer = true;
    TrackingStartTime = GetWorld()->GetTimeSeconds();

    // Reset pattern data
    CurrentPlayerPattern = FPlayerBehaviorPattern();

    UE_LOG(LogTemp, Log, TEXT("Elite AI started tracking player: %s"), *Player->GetName());
}

void UEliteAIIntelligenceComponent::StopPlayerTracking()
{
    TrackedPlayer = nullptr;
    bIsTrackingPlayer = false;
    UE_LOG(LogTemp, Log, TEXT("Elite AI stopped tracking player"));
}

void UEliteAIIntelligenceComponent::RecordPlayerPosition(const FVector& Position)
{
    if (!bIsTrackingPlayer || CurrentSettings.PatternAnalysisDepth <= 0.0f) {
        return;
    }

    CurrentPlayerPattern.RecentPositions.Add(Position);

    // Keep only recent positions (based on analysis depth)
    int32 MaxPositions = FMath::FloorToInt(CurrentSettings.PatternAnalysisDepth);
    if (CurrentPlayerPattern.RecentPositions.Num() > MaxPositions) {
        CurrentPlayerPattern.RecentPositions.RemoveAt(0);
    }

    // Update average movement speed
    if (CurrentPlayerPattern.RecentPositions.Num() >= 2) {
        FVector LastPos = CurrentPlayerPattern.RecentPositions[CurrentPlayerPattern.RecentPositions.Num() - 2];
        float Distance = FVector::Dist(Position, LastPos);
        float DeltaTime = 0.1f; // Assuming 10 FPS tick rate
        float Speed = Distance / DeltaTime;

        CurrentPlayerPattern.AverageMovementSpeed = (CurrentPlayerPattern.AverageMovementSpeed + Speed) * 0.5f;
    }
}

void UEliteAIIntelligenceComponent::RecordPlayerAttack(const FVector& AttackPosition, float Timing)
{
    if (!bIsTrackingPlayer) {
        return;
    }

    CurrentPlayerPattern.AttackPositions.Add(AttackPosition);
    CurrentPlayerPattern.AttackTimings.Add(Timing);

    // Calculate attack frequency
    if (CurrentPlayerPattern.AttackTimings.Num() >= 2) {
        float TimeDiff = CurrentPlayerPattern.AttackTimings.Last() - CurrentPlayerPattern.AttackTimings[CurrentPlayerPattern.AttackTimings.Num() - 2];
        CurrentPlayerPattern.AttackFrequency = 1.0f / FMath::Max(TimeDiff, 0.1f);
    }

    // Update preferred engagement distance
    if (OwnerPawn && CurrentPlayerPattern.AttackPositions.Num() > 0) {
        float TotalDistance = 0.0f;
        for (const FVector& Pos : CurrentPlayerPattern.AttackPositions) {
            TotalDistance += FVector::Dist(OwnerPawn->GetActorLocation(), Pos);
        }
        CurrentPlayerPattern.PreferredEngagementDistance = TotalDistance / CurrentPlayerPattern.AttackPositions.Num();
    }
}

void UEliteAIIntelligenceComponent::RecordPlayerDodge(const FVector& DodgeDirection)
{
    if (!bIsTrackingPlayer) {
        return;
    }

    CurrentPlayerPattern.DodgeDirections.Add(DodgeDirection);

    // Calculate preferred dodge direction
    if (CurrentPlayerPattern.DodgeDirections.Num() > 0) {
        FVector TotalDirection = FVector::ZeroVector;
        for (const FVector& Dir : CurrentPlayerPattern.DodgeDirections) {
            TotalDirection += Dir;
        }
        CurrentPlayerPattern.PreferredDodgeDirection = TotalDirection / CurrentPlayerPattern.DodgeDirections.Num();
    }
}

// Prediction Functions
FVector UEliteAIIntelligenceComponent::PredictPlayerPosition(float PredictionTime)
{
    if (!TrackedPlayer || !bIsTrackingPlayer || CurrentSettings.PredictionAccuracy <= 0.0f) {
        return TrackedPlayer ? TrackedPlayer->GetActorLocation() : FVector::ZeroVector;
    }

    FVector CurrentPos = TrackedPlayer->GetActorLocation();
    FVector CurrentVel = TrackedPlayer->GetVelocity();

    // Basic linear prediction
    FVector BasicPrediction = CurrentPos + (CurrentVel * PredictionTime);

    // Enhanced prediction based on difficulty
    if (CurrentSettings.PredictionAccuracy > 0.5f && CurrentPlayerPattern.RecentPositions.Num() > 3) {
        // Acceleration-based prediction
        FVector LastVel = (CurrentPlayerPattern.RecentPositions.Last() - CurrentPlayerPattern.RecentPositions[CurrentPlayerPattern.RecentPositions.Num() - 2]) / (1.0f / 60.0f);

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

    LastPredictionTime = GetWorld()->GetTimeSeconds();
    return CachedPlayerPositionPrediction;
}

FVector UEliteAIIntelligenceComponent::PredictPlayerMovement(float TimeAhead)
{
    return PredictPlayerPosition(TimeAhead);
}

bool UEliteAIIntelligenceComponent::ShouldPredictDodge(const FVector& AttackDirection)
{
    if (!bIsTrackingPlayer || CurrentSettings.PredictionAccuracy < 0.3f) {
        return false;
    }

    // Check if player has consistent dodge patterns
    if (CurrentPlayerPattern.DodgeDirections.Num() >= 3) {
        // Calculate pattern consistency
        float DotSum = 0.0f;
        for (int32 i = 0; i < CurrentPlayerPattern.DodgeDirections.Num() - 1; ++i) {
            DotSum += FVector::DotProduct(CurrentPlayerPattern.DodgeDirections[i],
                CurrentPlayerPattern.DodgeDirections[i + 1]);
        }
        float Consistency = DotSum / (CurrentPlayerPattern.DodgeDirections.Num() - 1);

        return Consistency > 0.5f; // 50% consistency threshold
    }

    return false;
}

// Adaptation Functions
void UEliteAIIntelligenceComponent::AdaptToPlayerBehavior()
{
    if (!bIsTrackingPlayer || AdaptationCooldown > 0.0f || CurrentSettings.AdaptationSpeed <= 0.0f) {
        return;
    }

    AnalyzePlayerMovement();
    AnalyzePlayerCombat();

    // Set adaptation cooldown
    AdaptationCooldown = 5.0f / CurrentSettings.AdaptationSpeed;

    UE_LOG(LogTemp, VeryVerbose, TEXT("Elite AI adapted to player behavior"));
}

void UEliteAIIntelligenceComponent::CounterPlayerStrategy()
{
    if (!CurrentSettings.bCanCounterAdapt || !bIsTrackingPlayer) {
        return;
    }

    // Counter circle strafing
    if (CurrentPlayerPattern.bPrefersCircleStrafing) {
        // Implement counter-strafing or prediction
        if (OwnerController) {
            OwnerController->ReceiveOverlordCommand("CounterStrafe",
                TArray<FVector> { CurrentPlayerPattern.PreferredDodgeDirection });
        }
    }

    // Counter cover usage
    if (CurrentPlayerPattern.bUsesEnvironmentCover) {
        // Implement flanking or area denial
        if (OwnerController) {
            OwnerController->ReceiveOverlordCommand("FlankCover", TArray<FVector>());
        }
    }
}

FString UEliteAIIntelligenceComponent::GetRecommendedTactic()
{
    if (!bIsTrackingPlayer) {
        return "Patrol";
    }

    // Analyze current situation and recommend tactics
    float EngagementDistance = CurrentPlayerPattern.PreferredEngagementDistance;

    if (EngagementDistance < 500.0f) {
        return "CloseQuarters";
    } else if (EngagementDistance > 1500.0f) {
        return "LongRange";
    } else if (CurrentPlayerPattern.bPrefersCircleStrafing) {
        return "CounterStrafe";
    } else if (CurrentPlayerPattern.bUsesEnvironmentCover) {
        return "FlankingManeuver";
    }

    return "StandardEngagement";
}

void UEliteAIIntelligenceComponent::RecordTacticSuccess(const FString& TacticName, bool bSuccessful)
{
    if (!TacticSuccessRates.Contains(TacticName)) {
        TacticSuccessRates.Add(TacticName, 0.5f); // Start with neutral rating
    }

    float& SuccessRate = TacticSuccessRates[TacticName];
    float Adjustment = bSuccessful ? 0.1f : -0.1f;
    SuccessRate = FMath::Clamp(SuccessRate + Adjustment, 0.0f, 1.0f);

    // Track recent tactics
    RecentTactics.Add(TacticName);
    if (RecentTactics.Num() > 10) {
        RecentTactics.RemoveAt(0);
    }
}

// Combat Intelligence
float UEliteAIIntelligenceComponent::CalculateOptimalEngagementDistance()
{
    if (!bIsTrackingPlayer) {
        return 1000.0f; // Default engagement distance
    }

    // Factor in player's preferred engagement distance
    float PlayerPreference = CurrentPlayerPattern.PreferredEngagementDistance;

    // Counter player preference
    if (CurrentSettings.bCanCounterAdapt) {
        if (PlayerPreference < 600.0f) {
            return PlayerPreference + 400.0f; // Stay at medium range if player prefers close
        } else {
            return PlayerPreference - 300.0f; // Close distance if player prefers long range
        }
    }

    return FMath::Clamp(PlayerPreference, 500.0f, 1500.0f);
}

FVector UEliteAIIntelligenceComponent::CalculateOptimalAttackPosition(const FVector& PlayerPosition)
{
    if (!OwnerPawn) {
        return PlayerPosition;
    }

    float OptimalDistance = CalculateOptimalEngagementDistance();
    FVector ToPlayer = (PlayerPosition - OwnerPawn->GetActorLocation()).GetSafeNormal();

    // Position at optimal distance
    FVector BasePosition = PlayerPosition - (ToPlayer * OptimalDistance);

    // Add tactical offset based on player patterns
    if (CurrentPlayerPattern.bPrefersCircleStrafing) {
        // Position to intercept strafing
        FVector PerpendicularOffset = FVector::CrossProduct(ToPlayer, FVector::UpVector) * 200.0f;
        BasePosition += PerpendicularOffset;
    }

    return BasePosition;
}

bool UEliteAIIntelligenceComponent::ShouldUseFlankingManeuver()
{
    if (CurrentSettings.bCanCounterAdapt && bIsTrackingPlayer) {
        // Use flanking if player uses cover frequently
        return CurrentPlayerPattern.bUsesEnvironmentCover;
    }

    return false;
}

bool UEliteAIIntelligenceComponent::ShouldRetreat()
{
    // Implement retreat logic based on tactical situation
    // This could factor in health, ammunition, tactical advantage, etc.
    return false; // Placeholder
}

// Configuration Functions
void UEliteAIIntelligenceComponent::UpdateEliteSettings()
{
    ApplyDifficultySettings();
}

void UEliteAIIntelligenceComponent::ResetBehaviorPatterns()
{
    CurrentPlayerPattern = FPlayerBehaviorPattern();
    TacticSuccessRates.Empty();
    RecentTactics.Empty();
    AdaptationCooldown = 0.0f;
    PredictionConfidence = 0.0f;
}

// Private Helper Functions
void UEliteAIIntelligenceComponent::UpdatePlayerPattern()
{
    if (!TrackedPlayer) {
        return;
    }

    // Record current position
    RecordPlayerPosition(TrackedPlayer->GetActorLocation());

    // Analyze movement patterns
    AnalyzePlayerMovement();
}

void UEliteAIIntelligenceComponent::UpdatePredictions()
{
    if (!bIsTrackingPlayer || CurrentSettings.PredictionAccuracy <= 0.0f) {
        return;
    }

    // Cache predictions for performance
    CachePredictions();

    // Update prediction confidence
    PredictionConfidence = CalculatePatternConfidence();
}

void UEliteAIIntelligenceComponent::UpdateAdaptation()
{
    if (!CurrentSettings.bCanCounterAdapt || AdaptationCooldown > 0.0f) {
        return;
    }

    // Periodic adaptation based on learned patterns
    if (IsPlayerBehaviorConsistent()) {
        CounterPlayerStrategy();
    }
}

void UEliteAIIntelligenceComponent::AnalyzePlayerMovement()
{
    if (CurrentPlayerPattern.RecentPositions.Num() < 5) {
        return;
    }

    // Detect circle strafing
    float CircularityScore = 0.0f;
    FVector CenterPoint = FVector::ZeroVector;

    // Calculate approximate center
    for (const FVector& Pos : CurrentPlayerPattern.RecentPositions) {
        CenterPoint += Pos;
    }
    CenterPoint /= CurrentPlayerPattern.RecentPositions.Num();

    // Check if movement forms circular pattern
    float AvgDistance = 0.0f;
    for (const FVector& Pos : CurrentPlayerPattern.RecentPositions) {
        AvgDistance += FVector::Dist(Pos, CenterPoint);
    }
    AvgDistance /= CurrentPlayerPattern.RecentPositions.Num();

    float DistanceVariance = 0.0f;
    for (const FVector& Pos : CurrentPlayerPattern.RecentPositions) {
        float Diff = FVector::Dist(Pos, CenterPoint) - AvgDistance;
        DistanceVariance += Diff * Diff;
    }
    DistanceVariance /= CurrentPlayerPattern.RecentPositions.Num();

    // Low variance in distance from center indicates circular movement
    CurrentPlayerPattern.bPrefersCircleStrafing = (DistanceVariance < 10000.0f && AvgDistance > 300.0f);
}

void UEliteAIIntelligenceComponent::AnalyzePlayerCombat()
{
    // Analyze combat patterns based on attack positions and timings
    if (CurrentPlayerPattern.AttackPositions.Num() >= 3) {
        // Check if player uses cover (attacks from similar positions)
        float PositionVariance = 0.0f;
        FVector AvgAttackPos = FVector::ZeroVector;

        for (const FVector& Pos : CurrentPlayerPattern.AttackPositions) {
            AvgAttackPos += Pos;
        }
        AvgAttackPos /= CurrentPlayerPattern.AttackPositions.Num();

        for (const FVector& Pos : CurrentPlayerPattern.AttackPositions) {
            PositionVariance += FVector::DistSquared(Pos, AvgAttackPos);
        }
        PositionVariance /= CurrentPlayerPattern.AttackPositions.Num();

        // Low variance indicates consistent cover usage
        CurrentPlayerPattern.bUsesEnvironmentCover = (PositionVariance < 250000.0f);
    }
}

void UEliteAIIntelligenceComponent::ApplyDifficultySettings()
{
    // Configure settings based on difficulty level
    switch (CurrentDifficultyLevel) {
    case EEliteDifficultyLevel::Disabled:
        CurrentSettings.ReactionTimeMultiplier = 1.0f;
        CurrentSettings.AccuracyMultiplier = 1.0f;
        CurrentSettings.PredictionAccuracy = 0.0f;
        CurrentSettings.AdaptationSpeed = 0.0f;
        CurrentSettings.bCanCounterAdapt = false;
        break;

    case EEliteDifficultyLevel::Novice:
        CurrentSettings.ReactionTimeMultiplier = 0.9f;
        CurrentSettings.AccuracyMultiplier = 1.1f;
        CurrentSettings.PredictionAccuracy = 0.2f;
        CurrentSettings.AdaptationSpeed = 0.5f;
        CurrentSettings.bCanCounterAdapt = false;
        break;

    case EEliteDifficultyLevel::Skilled:
        CurrentSettings.ReactionTimeMultiplier = 0.8f;
        CurrentSettings.AccuracyMultiplier = 1.2f;
        CurrentSettings.PredictionAccuracy = 0.4f;
        CurrentSettings.AdaptationSpeed = 0.8f;
        CurrentSettings.bCanCounterAdapt = false;
        break;

    case EEliteDifficultyLevel::Veteran:
        CurrentSettings.ReactionTimeMultiplier = 0.7f;
        CurrentSettings.AccuracyMultiplier = 1.3f;
        CurrentSettings.PredictionAccuracy = 0.6f;
        CurrentSettings.AdaptationSpeed = 1.0f;
        CurrentSettings.bCanCounterAdapt = true;
        break;

    case EEliteDifficultyLevel::Expert:
        CurrentSettings.ReactionTimeMultiplier = 0.6f;
        CurrentSettings.AccuracyMultiplier = 1.4f;
        CurrentSettings.PredictionAccuracy = 0.7f;
        CurrentSettings.AdaptationSpeed = 1.2f;
        CurrentSettings.bCanCounterAdapt = true;
        break;

    case EEliteDifficultyLevel::Master:
        CurrentSettings.ReactionTimeMultiplier = 0.5f;
        CurrentSettings.AccuracyMultiplier = 1.5f;
        CurrentSettings.PredictionAccuracy = 0.8f;
        CurrentSettings.AdaptationSpeed = 1.5f;
        CurrentSettings.bCanCounterAdapt = true;
        CurrentSettings.bUsePsychologicalWarfare = true;
        break;

    case EEliteDifficultyLevel::Grandmaster:
        CurrentSettings.ReactionTimeMultiplier = 0.4f;
        CurrentSettings.AccuracyMultiplier = 1.6f;
        CurrentSettings.PredictionAccuracy = 0.85f;
        CurrentSettings.AdaptationSpeed = 1.8f;
        CurrentSettings.bCanCounterAdapt = true;
        CurrentSettings.bUsePsychologicalWarfare = true;
        break;

    case EEliteDifficultyLevel::Legend:
        CurrentSettings.ReactionTimeMultiplier = 0.3f;
        CurrentSettings.AccuracyMultiplier = 1.7f;
        CurrentSettings.PredictionAccuracy = 0.9f;
        CurrentSettings.AdaptationSpeed = 2.0f;
        CurrentSettings.bCanCounterAdapt = true;
        CurrentSettings.bUsePsychologicalWarfare = true;
        break;

    case EEliteDifficultyLevel::Nightmare:
        CurrentSettings.ReactionTimeMultiplier = 0.25f;
        CurrentSettings.AccuracyMultiplier = 1.8f;
        CurrentSettings.PredictionAccuracy = 0.93f;
        CurrentSettings.AdaptationSpeed = 2.5f;
        CurrentSettings.bCanCounterAdapt = true;
        CurrentSettings.bUsePsychologicalWarfare = true;
        break;

    case EEliteDifficultyLevel::Impossible:
        CurrentSettings.ReactionTimeMultiplier = 0.2f;
        CurrentSettings.AccuracyMultiplier = 1.9f;
        CurrentSettings.PredictionAccuracy = 0.95f;
        CurrentSettings.AdaptationSpeed = 3.0f;
        CurrentSettings.bCanCounterAdapt = true;
        CurrentSettings.bUsePsychologicalWarfare = true;
        break;

    case EEliteDifficultyLevel::Godlike:
        CurrentSettings.ReactionTimeMultiplier = 0.1f;
        CurrentSettings.AccuracyMultiplier = 2.0f;
        CurrentSettings.PredictionAccuracy = 0.98f;
        CurrentSettings.AdaptationSpeed = 4.0f;
        CurrentSettings.bCanCounterAdapt = true;
        CurrentSettings.bUsePsychologicalWarfare = true;
        break;
    }

    // Set pattern analysis depth based on difficulty
    CurrentSettings.PatternAnalysisDepth = 5.0f + (float)CurrentDifficultyLevel * 2.0f;
}

void UEliteAIIntelligenceComponent::CachePredictions()
{
    // Cache expensive predictions for performance
    if (TrackedPlayer) {
        CachedPlayerPositionPrediction = PredictPlayerPosition(1.0f);
    }
}

bool UEliteAIIntelligenceComponent::IsPlayerBehaviorConsistent() const
{
    return CalculatePatternConfidence() > 0.7f;
}

float UEliteAIIntelligenceComponent::CalculatePatternConfidence() const
{
    if (CurrentPlayerPattern.RecentPositions.Num() < 5) {
        return 0.0f;
    }

    float Confidence = 0.0f;

    // Factor in position consistency
    if (CurrentPlayerPattern.bPrefersCircleStrafing || CurrentPlayerPattern.bUsesEnvironmentCover) {
        Confidence += 0.3f;
    }

    // Factor in attack pattern consistency
    if (CurrentPlayerPattern.AttackPositions.Num() >= 3) {
        Confidence += 0.3f;
    }

    // Factor in dodge pattern consistency
    if (CurrentPlayerPattern.DodgeDirections.Num() >= 3) {
        Confidence += 0.4f;
    }

    return FMath::Clamp(Confidence, 0.0f, 1.0f);
}