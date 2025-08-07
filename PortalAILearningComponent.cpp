// PortalAILearningComponent.cpp
#include "PortalAILearningComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PortalACFAIController.h"

// Static member initialization
TMap<FString, FPlayerBehaviorProfile> UPortalAILearningComponent::GlobalPlayerDatabase;

UPortalAILearningComponent::UPortalAILearningComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Set default configuration
    bEnableLearning = true;
    bPersistLearningData = true;
    LearningRate = 0.1f;
    MinObservationsForPattern = 3;
    PatternRecognitionThreshold = 0.7f;
    MemoryRetentionTime = 300.0f;
    MaxStoredPatterns = 20;
    MaxPositionHistory = 50;
}

void UPortalAILearningComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bEnableLearning && bPersistLearningData) {
        LoadLearningData();
    }
}

void UPortalAILearningComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bEnableLearning && bPersistLearningData) {
        SaveLearningData();
    }

    Super::EndPlay(EndPlayReason);
}

void UPortalAILearningComponent::Initialize(APortalACFAIController* Controller)
{
    OwnerController = Controller;

    if (!OwnerController) {
        UE_LOG(LogTemp, Warning, TEXT("PortalAILearningComponent: No valid controller provided"));
        return;
    }

    StartLearning();

    UE_LOG(LogTemp, Log, TEXT("Learning Component initialized"));
}

void UPortalAILearningComponent::StartLearning()
{
    if (!bEnableLearning) {
        return;
    }

    bIsLearning = true;
    CurrentSession.SessionStartTime = GetWorld()->GetTimeSeconds();

    UE_LOG(LogTemp, Log, TEXT("AI Learning started"));
}

void UPortalAILearningComponent::StopLearning()
{
    bIsLearning = false;

    CurrentSession.SessionDuration = GetWorld()->GetTimeSeconds() - CurrentSession.SessionStartTime;

    // Calculate session score
    if (TotalPredictions > 0) {
        PredictionAccuracy = (float)CorrectPredictions / (float)TotalPredictions;
        CurrentSession.AdaptationScore = PredictionAccuracy;
    }

    UE_LOG(LogTemp, Log, TEXT("AI Learning stopped. Session duration: %.2f, Adaptation: %.2f"),
        CurrentSession.SessionDuration, CurrentSession.AdaptationScore);
}

void UPortalAILearningComponent::ResetLearning()
{
    CurrentPlayerProfile = FPlayerBehaviorProfile();
    CurrentSession = FLearningSessionData();
    RecentPlayerActions.Empty();
    RecentPlayerPositions.Empty();
    ActionTimestamps.Empty();
    CorrectPredictions = 0;
    TotalPredictions = 0;
    PredictionAccuracy = 0.0f;
    AdaptationLevel = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("AI Learning data reset"));
}

void UPortalAILearningComponent::ProcessCombatData(AActor* Target, float DeltaTime)
{
    if (!bIsLearning || !Target) {
        return;
    }

    UpdatePlayerProfile(Target, DeltaTime);
    ProcessPatternRecognition();
    CalculateAdaptationLevel();
    ApplyLearningToController();

    // Clean up old data periodically
    if (FMath::FRand() < 0.1f) // 10% chance per update
    {
        CleanupOldData();
    }
}

void UPortalAILearningComponent::RecordPlayerAction(FName ActionType, const FVector& ActionLocation)
{
    if (!bIsLearning) {
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();

    // Add to recent actions
    RecentPlayerActions.Add(ActionType);
    if (RecentPlayerActions.Num() > MaxStoredPatterns) {
        RecentPlayerActions.RemoveAt(0);
    }

    // Record position
    RecentPlayerPositions.Add(ActionLocation);
    if (RecentPlayerPositions.Num() > MaxPositionHistory) {
        RecentPlayerPositions.RemoveAt(0);
    }

    // Update action timestamps
    ActionTimestamps.Add(ActionType, CurrentTime);

    // Update action frequencies
    UpdateActionFrequencies(ActionType);

    CurrentSession.ActionsObserved++;
    LastActionTime = CurrentTime;
}

void UPortalAILearningComponent::RecordCombatOutcome(bool bPlayerWon)
{
    if (!bIsLearning) {
        return;
    }

    CurrentPlayerProfile.TotalEncounters++;

    if (bPlayerWon) {
        CurrentPlayerProfile.PlayerVictories++;
    } else {
        CurrentPlayerProfile.AIVictories++;
    }

    // Adjust learning rate based on outcome
    if (!bPlayerWon) {
        // We won, our predictions were likely good
        LearningRate *= 0.95f; // Slow down learning slightly
        CorrectPredictions++;
    } else {
        // We lost, need to learn more
        LearningRate = FMath::Min(LearningRate * 1.1f, 0.5f); // Speed up learning
    }

    TotalPredictions++;

    UE_LOG(LogTemp, Verbose, TEXT("Combat outcome recorded. Win rate: %.2f%%"),
        (float)CurrentPlayerProfile.AIVictories / (float)CurrentPlayerProfile.TotalEncounters * 100.0f);
}

void UPortalAILearningComponent::OnCombatStarted(AActor* Target)
{
    if (!bIsLearning || !Target) {
        return;
    }

    CombatStartTime = GetWorld()->GetTimeSeconds();

    // Load existing profile for this player if available
    FString PlayerID = GetPlayerID(Target);
    if (GlobalPlayerDatabase.Contains(PlayerID)) {
        CurrentPlayerProfile = GlobalPlayerDatabase[PlayerID];
        UE_LOG(LogTemp, Log, TEXT("Loaded existing player profile for %s"), *PlayerID);
    } else {
        CurrentPlayerProfile = FPlayerBehaviorProfile();
        CurrentPlayerProfile.PlayerID = PlayerID;
        UE_LOG(LogTemp, Log, TEXT("Created new player profile for %s"), *PlayerID);
    }
}

void UPortalAILearningComponent::OnCombatEnded()
{
    if (!bIsLearning) {
        return;
    }

    float CombatDuration = GetWorld()->GetTimeSeconds() - CombatStartTime;

    // Save profile to global database
    if (!CurrentPlayerProfile.PlayerID.IsEmpty()) {
        GlobalPlayerDatabase.Add(CurrentPlayerProfile.PlayerID, CurrentPlayerProfile);
    }

    UE_LOG(LogTemp, Log, TEXT("Combat ended. Duration: %.2f seconds"), CombatDuration);
}

bool UPortalAILearningComponent::DetectPattern(const TArray<FName>& RecentActions, FCombatPattern& OutPattern)
{
    if (RecentActions.Num() < MinObservationsForPattern) {
        return false;
    }

    // Simple pattern detection - look for repeated sequences
    for (int32 PatternLength = 2; PatternLength <= FMath::Min(5, RecentActions.Num() / 2); PatternLength++) {
        for (int32 StartIdx = 0; StartIdx <= RecentActions.Num() - PatternLength * 2; StartIdx++) {
            bool bPatternFound = true;

            // Check if the pattern repeats
            for (int32 i = 0; i < PatternLength; i++) {
                if (RecentActions[StartIdx + i] != RecentActions[StartIdx + PatternLength + i]) {
                    bPatternFound = false;
                    break;
                }
            }

            if (bPatternFound) {
                // Create pattern name from actions
                FString PatternString;
                for (int32 i = 0; i < PatternLength; i++) {
                    PatternString += RecentActions[StartIdx + i].ToString() + "_";
                }

                OutPattern.PatternName = FName(*PatternString);
                OutPattern.TimesObserved++;

                // Check if this pattern already exists
                if (FCombatPattern* ExistingPattern = CurrentPlayerProfile.KnownPatterns.Find(OutPattern.PatternName)) {
                    ExistingPattern->TimesObserved++;
                    OutPattern = *ExistingPattern;
                } else {
                    CurrentPlayerProfile.KnownPatterns.Add(OutPattern.PatternName, OutPattern);
                    CurrentSession.PatternsLearned++;
                }

                OnPatternRecognized(OutPattern);
                return true;
            }
        }
    }

    return false;
}

FCombatPattern UPortalAILearningComponent::GetKnownPattern(FName PatternName) const
{
    if (const FCombatPattern* Pattern = CurrentPlayerProfile.KnownPatterns.Find(PatternName)) {
        return *Pattern;
    }

    return FCombatPattern();
}

TArray<FCombatPattern> UPortalAILearningComponent::GetAllKnownPatterns() const
{
    TArray<FCombatPattern> Patterns;

    for (const auto& Pair : CurrentPlayerProfile.KnownPatterns) {
        Patterns.Add(Pair.Value);
    }

    return Patterns;
}

float UPortalAILearningComponent::GetPatternPrediction(FName PatternName) const
{
    if (const FCombatPattern* Pattern = CurrentPlayerProfile.KnownPatterns.Find(PatternName)) {
        if (Pattern->TimesObserved > 0) {
            return Pattern->SuccessRate;
        }
    }

    return 0.0f;
}

FName UPortalAILearningComponent::GetCounterAction(FName PlayerAction) const
{
    // Define counter actions based on learned patterns
    // This is a simplified implementation - in production you'd have more sophisticated counters

    if (PlayerAction == "Attack") {
        if (CurrentPlayerProfile.BlockFrequency > 0.5f) {
            return "Dodge"; // Player blocks often, so dodge instead
        }
        return "Block";
    } else if (PlayerAction == "Block") {
        return "HeavyAttack"; // Break through block
    } else if (PlayerAction == "Dodge") {
        return "DelayedAttack"; // Wait for dodge to end
    }

    return "Defend"; // Default defensive action
}

FVector UPortalAILearningComponent::PredictPlayerPosition(float TimeInFuture) const
{
    if (RecentPlayerPositions.Num() < 2) {
        return RecentPlayerPositions.Num() > 0 ? RecentPlayerPositions.Last() : FVector::ZeroVector;
    }

    // Simple linear extrapolation based on recent movement
    FVector LastPos = RecentPlayerPositions.Last();
    FVector SecondLastPos = RecentPlayerPositions[RecentPlayerPositions.Num() - 2];
    FVector Velocity = (LastPos - SecondLastPos) / 0.1f; // Assume 0.1s between samples

    return LastPos + (Velocity * TimeInFuture);
}

float UPortalAILearningComponent::GetOptimalCombatRange() const
{
    return CurrentPlayerProfile.PreferredCombatRange;
}

float UPortalAILearningComponent::GetAdaptedReactionTime() const
{
    // Faster reaction time as we learn more about the player
    float BaseReactionTime = 0.5f;
    float AdaptationBonus = AdaptationLevel * 0.3f; // Up to 30% faster

    return FMath::Max(0.1f, BaseReactionTime - AdaptationBonus);
}

void UPortalAILearningComponent::SaveLearningData()
{
    // In a real implementation, this would save to a file or database
    // For now, just log that we're saving

    UE_LOG(LogTemp, Log, TEXT("Saving learning data for %d player profiles"), GlobalPlayerDatabase.Num());

    // The GlobalPlayerDatabase persists across AI instances
    // Individual AI can access shared knowledge
}

void UPortalAILearningComponent::LoadLearningData()
{
    // In a real implementation, this would load from a file or database
    // For now, just log that we're loading

    UE_LOG(LogTemp, Log, TEXT("Loading learning data. Found %d existing player profiles"), GlobalPlayerDatabase.Num());
}

void UPortalAILearningComponent::ShareKnowledge(UPortalAILearningComponent* OtherAI)
{
    if (!OtherAI) {
        return;
    }

    // Share known patterns
    for (const auto& Pattern : CurrentPlayerProfile.KnownPatterns) {
        if (!OtherAI->CurrentPlayerProfile.KnownPatterns.Contains(Pattern.Key)) {
            OtherAI->CurrentPlayerProfile.KnownPatterns.Add(Pattern.Key, Pattern.Value);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Shared %d patterns with other AI"), CurrentPlayerProfile.KnownPatterns.Num());
}

void UPortalAILearningComponent::UpdatePlayerProfile(AActor* Player, float DeltaTime)
{
    if (!Player) {
        return;
    }

    // Update movement analysis
    AnalyzeMovementPattern(Player->GetActorLocation());

    // Update combat range preference
    float CurrentDistance = FVector::Dist(OwnerController->GetPawn()->GetActorLocation(), Player->GetActorLocation());
    CurrentPlayerProfile.PreferredCombatRange = FMath::Lerp(
        CurrentPlayerProfile.PreferredCombatRange,
        CurrentDistance,
        LearningRate * DeltaTime);

    // Update movement speed if player is a character
    if (ACharacter* PlayerChar = Cast<ACharacter>(Player)) {
        if (UCharacterMovementComponent* MoveComp = PlayerChar->GetCharacterMovement()) {
            float CurrentSpeed = MoveComp->Velocity.Size();
            CurrentPlayerProfile.AverageMovementSpeed = FMath::Lerp(
                CurrentPlayerProfile.AverageMovementSpeed,
                CurrentSpeed,
                LearningRate * DeltaTime);
        }
    }

    OnPlayerProfileUpdated(CurrentPlayerProfile);
}

void UPortalAILearningComponent::AnalyzeMovementPattern(const FVector& PlayerLocation)
{
    // Add to frequent positions if player stays in area
    bool bFoundNearbyPosition = false;
    const float PositionThreshold = 200.0f;

    for (FVector& FrequentPos : CurrentPlayerProfile.FrequentPositions) {
        if (FVector::Dist(FrequentPos, PlayerLocation) < PositionThreshold) {
            // Update average position
            FrequentPos = FMath::Lerp(FrequentPos, PlayerLocation, 0.1f);
            bFoundNearbyPosition = true;
            break;
        }
    }

    if (!bFoundNearbyPosition && CurrentPlayerProfile.FrequentPositions.Num() < 10) {
        CurrentPlayerProfile.FrequentPositions.Add(PlayerLocation);
    }
}

void UPortalAILearningComponent::UpdateActionFrequencies(FName ActionType)
{
    const float DecayRate = 0.95f; // Decay old frequencies

    // Decay all frequencies
    CurrentPlayerProfile.DodgeFrequency *= DecayRate;
    CurrentPlayerProfile.BlockFrequency *= DecayRate;
    CurrentPlayerProfile.AttackFrequency *= DecayRate;

    // Update the specific action frequency
    if (ActionType.ToString().Contains("Dodge")) {
        CurrentPlayerProfile.DodgeFrequency = FMath::Min(1.0f, CurrentPlayerProfile.DodgeFrequency + LearningRate);
    } else if (ActionType.ToString().Contains("Block")) {
        CurrentPlayerProfile.BlockFrequency = FMath::Min(1.0f, CurrentPlayerProfile.BlockFrequency + LearningRate);
    } else if (ActionType.ToString().Contains("Attack")) {
        CurrentPlayerProfile.AttackFrequency = FMath::Min(1.0f, CurrentPlayerProfile.AttackFrequency + LearningRate);
    }
}

void UPortalAILearningComponent::ProcessPatternRecognition()
{
    FCombatPattern DetectedPattern;
    if (DetectPattern(RecentPlayerActions, DetectedPattern)) {
        // Pattern detected, update success metrics
        if (DetectedPattern.TimesObserved >= MinObservationsForPattern) {
            DetectedPattern.SuccessRate = (float)DetectedPattern.SuccessfulCounters / (float)DetectedPattern.TimesObserved;

            // Store updated pattern
            CurrentPlayerProfile.KnownPatterns.Add(DetectedPattern.PatternName, DetectedPattern);
        }
    }
}

void UPortalAILearningComponent::CalculateAdaptationLevel()
{
    float NewAdaptationLevel = 0.0f;

    // Factor 1: Pattern recognition (30%)
    float PatternScore = 0.0f;
    if (CurrentPlayerProfile.KnownPatterns.Num() > 0) {
        PatternScore = FMath::Min(1.0f, (float)CurrentPlayerProfile.KnownPatterns.Num() / 10.0f);
    }
    NewAdaptationLevel += PatternScore * 0.3f;

    // Factor 2: Prediction accuracy (40%)
    NewAdaptationLevel += PredictionAccuracy * 0.4f;

    // Factor 3: Combat experience (30%)
    float ExperienceScore = FMath::Min(1.0f, (float)CurrentPlayerProfile.TotalEncounters / 20.0f);
    NewAdaptationLevel += ExperienceScore * 0.3f;

    // Smooth the adaptation level change
    AdaptationLevel = FMath::Lerp(AdaptationLevel, NewAdaptationLevel, 0.1f);

    OnAdaptationLevelChanged(AdaptationLevel);
}

void UPortalAILearningComponent::CleanupOldData()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();

    // Remove old action timestamps
    TArray<FName> ActionsToRemove;
    for (const auto& Pair : ActionTimestamps) {
        if (CurrentTime - Pair.Value > MemoryRetentionTime) {
            ActionsToRemove.Add(Pair.Key);
        }
    }

    for (const FName& Action : ActionsToRemove) {
        ActionTimestamps.Remove(Action);
    }

    // Limit stored patterns
    if (CurrentPlayerProfile.KnownPatterns.Num() > MaxStoredPatterns) {
        // Remove least observed patterns
        TArray<FName> PatternNames;
        CurrentPlayerProfile.KnownPatterns.GetKeys(PatternNames);

        PatternNames.Sort([this](const FName& A, const FName& B) {
            const FCombatPattern* PatternA = CurrentPlayerProfile.KnownPatterns.Find(A);
            const FCombatPattern* PatternB = CurrentPlayerProfile.KnownPatterns.Find(B);
            return PatternA->TimesObserved > PatternB->TimesObserved;
        });

        // Keep only the most observed patterns
        while (PatternNames.Num() > MaxStoredPatterns) {
            CurrentPlayerProfile.KnownPatterns.Remove(PatternNames.Pop());
        }
    }
}

void UPortalAILearningComponent::ApplyLearningToController()
{
    if (!OwnerController || !OwnerController->GetBlackboardComponent()) {
        return;
    }

    UBlackboardComponent* Blackboard = OwnerController->GetBlackboardComponent();

    // Update blackboard with learned data
    Blackboard->SetValueAsFloat("AdaptationLevel", AdaptationLevel);
    Blackboard->SetValueAsFloat("PlayerDodgeFrequency", CurrentPlayerProfile.DodgeFrequency);
    Blackboard->SetValueAsFloat("PlayerBlockFrequency", CurrentPlayerProfile.BlockFrequency);
    Blackboard->SetValueAsFloat("PlayerAttackFrequency", CurrentPlayerProfile.AttackFrequency);
    Blackboard->SetValueAsFloat("OptimalCombatRange", GetOptimalCombatRange());
    Blackboard->SetValueAsFloat("AdaptedReactionTime", GetAdaptedReactionTime());

    // Set next predicted action if we have high confidence
    if (RecentPlayerActions.Num() > 0 && AdaptationLevel > 0.5f) {
        FName PredictedAction = GetCounterAction(RecentPlayerActions.Last());
        Blackboard->SetValueAsName("PredictedPlayerAction", PredictedAction);
    }
}

FString UPortalAILearningComponent::GetPlayerID(AActor* Player) const
{
    if (!Player) {
        return "";
    }

    // In a real implementation, this would get the actual player ID
    // For now, use the player's name
    return Player->GetName();
}