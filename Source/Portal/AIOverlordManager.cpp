#include "AIOverlordManager.h"
#include "ACFAIController.h"
#include "Components/ACFCommandsManagerComponent.h"
#include "Components/ACFThreatManagerComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "PortalCore.h"
#include "PortalDefenseAIController.h"

UAIOverlordManager::UAIOverlordManager()
{
    AIIntelligenceLevel = 1.0f;
    IntelligenceGrowthRate = 0.1f;
    MaxIntelligenceLevel = 5.0f;
    AnalysisInterval = 5.0f;
    PlayerTrackingInterval = 1.0f;
    bEnableContinuousAnalysis = true;
    MaxPlayerPositionHistory = 100;
    TotalPlayerIncursions = 0;

    // Setup ACF integration tags
    PatrolCommandTag = FGameplayTag::RequestGameplayTag(FName("AI.Commands.Patrol"));
    AlertCommandTag = FGameplayTag::RequestGameplayTag(FName("AI.Commands.Alert"));
    CoordinateCommandTag = FGameplayTag::RequestGameplayTag(FName("AI.Commands.Coordinate"));
    DefaultAIState = FGameplayTag::RequestGameplayTag(FName("AI.State.Default"));
    PatrolAIState = FGameplayTag::RequestGameplayTag(FName("AI.State.Patrol"));
}

void UAIOverlordManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FindPortalTarget();
    SessionStartTime = GetWorld()->GetTimeSeconds();

    if (bEnableContinuousAnalysis) {
        StartContinuousAnalysis();
    }
}

void UAIOverlordManager::Deinitialize()
{
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(AnalysisTimer);
        World->GetTimerManager().ClearTimer(PlayerTrackingTimer);
    }

    RegisteredAI.Empty();
    AnalysisHistory.Empty();

    Super::Deinitialize();
}

bool UAIOverlordManager::ShouldCreateSubsystem(UObject* Outer) const
{
    return Super::ShouldCreateSubsystem(Outer);
}

UAIOverlordManager* UAIOverlordManager::GetInstance(const UObject* WorldContext)
{
    if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull)) {
        return World->GetSubsystem<UAIOverlordManager>();
    }
    return nullptr;
}

void UAIOverlordManager::RegisterAI(AACFAIController* AIController)
{
    if (AIController && !RegisteredAI.Contains(AIController)) {
        RegisteredAI.Add(AIController);

        CurrentAnalysisData.ActivePatrolGuards = RegisteredAI.Num();

        UE_LOG(LogTemp, Log, TEXT("AI Overlord: Registered patrol guard %s"), *AIController->GetName());
    }
}

void UAIOverlordManager::UnregisterAI(AACFAIController* AIController)
{
    if (AIController) {
        RegisteredAI.Remove(AIController);
        CurrentAnalysisData.ActivePatrolGuards = RegisteredAI.Num();
        UE_LOG(LogTemp, Log, TEXT("AI Overlord: Unregistered patrol guard %s"), *AIController->GetName());
    }
}

void UAIOverlordManager::AnalyzePatrolPerformance()
{
    CurrentAnalysisData.SessionDuration = GetWorld()->GetTimeSeconds() - SessionStartTime;
    CurrentAnalysisData.PlayerIncursions = TotalPlayerIncursions;
    CurrentAnalysisData.PlayerPositions = RecentPlayerPositions;

    // Calculate average detection time
    float TotalDetectionTime = 0.0f;
    int32 ValidDetections = 0;

    for (AACFAIController* AI : RegisteredAI) {
        if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AI)) {
            // Could track detection times here if exposed
            ValidDetections++;
        }
    }

    CurrentAnalysisData.AveragePlayerDetectionTime = ValidDetections > 0 ? TotalDetectionTime / ValidDetections : 0.0f;

    AnalysisHistory.Add(CurrentAnalysisData);

    // Increase intelligence based on performance
    float IntelligenceGain = IntelligenceGrowthRate;
    if (TotalPlayerIncursions > 3) {
        IntelligenceGain *= 1.3f; // Learn from frequent incursions
    }

    AIIntelligenceLevel = FMath::Min(AIIntelligenceLevel + IntelligenceGain, MaxIntelligenceLevel);

    UE_LOG(LogTemp, Log, TEXT("AI Overlord: Patrol analysis complete. Intelligence Level: %.2f, Incursions: %d"),
        AIIntelligenceLevel, TotalPlayerIncursions);
}

void UAIOverlordManager::RecordAIDeath(AACFAIController* DeadAI, FVector DeathLocation)
{
    if (DeadAI) {
        CurrentAnalysisData.GuardDeathLocations.Add(DeathLocation);
        UnregisterAI(DeadAI);

        // Alert nearby guards about the death
        AlertNearbyGuards(DeathLocation, 2000.0f);

        // Increase intelligence from learning about vulnerabilities
        AIIntelligenceLevel = FMath::Min(AIIntelligenceLevel + 0.05f, MaxIntelligenceLevel);
    }
}

void UAIOverlordManager::RecordPlayerPosition(FVector PlayerLocation)
{
    RecentPlayerPositions.Add(PlayerLocation);

    if (RecentPlayerPositions.Num() > MaxPlayerPositionHistory) {
        RecentPlayerPositions.RemoveAt(0);
    }
}

void UAIOverlordManager::RecordPlayerIncursion(FVector IncursionLocation)
{
    TotalPlayerIncursions++;
    PlayerIncursionPoints.Add(IncursionLocation);

    // Alert guards about incursion
    AlertNearbyGuards(IncursionLocation, 1500.0f);

    UE_LOG(LogTemp, Log, TEXT("AI Overlord: Player incursion recorded at location: %s"), *IncursionLocation.ToString());
}

void UAIOverlordManager::UpdateCaptureProgress(float Progress)
{
    CurrentAnalysisData.CaptureProgress = Progress;

    // Increase urgency if capture progress is high
    if (Progress > 0.5f) {
        IssueGlobalCommand("IncreaseAggression", TArray<FVector>());
    }
}

void UAIOverlordManager::UpgradePatrolAI()
{
    FACFAIUpgradeData UpgradeData = CalculateAIUpgrades(AIIntelligenceLevel);

    CleanupInvalidAI();

    for (AACFAIController* AI : RegisteredAI) {
        if (AI && IsValid(AI)) {
            SetACFPatrolBehavior(AI, UpgradeData);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AI Overlord: Upgraded %d patrol guards"), RegisteredAI.Num());
}

void UAIOverlordManager::AssignPatrolCoordination()
{
    CleanupInvalidAI();

    if (RegisteredAI.Num() < 2) {
        return;
    }

    // Enable coordination for AI above certain intelligence level
    if (AIIntelligenceLevel >= 2.0f) {
        for (AACFAIController* AI : RegisteredAI) {
            if (AI && IsValid(AI) && CoordinateCommandTag.IsValid()) {
                SendACFCommand(AI, CoordinateCommandTag);
            }
        }
    }
}

void UAIOverlordManager::OptimizePatrolRoutes()
{
    if (!PortalTarget) {
        return;
    }

    FVector PortalLocation = PortalTarget->GetActorLocation();

    // Analyze player incursion patterns and adjust patrol routes
    if (PlayerIncursionPoints.Num() > 0) {
        // Find most common incursion areas
        TMap<FVector, int32> IncursionHeatMap;
        float GridSize = 500.0f;

        for (const FVector& Incursion : PlayerIncursionPoints) {
            FVector GridPoint = FVector(
                FMath::RoundToFloat(Incursion.X / GridSize) * GridSize,
                FMath::RoundToFloat(Incursion.Y / GridSize) * GridSize,
                Incursion.Z);

            IncursionHeatMap.FindOrAdd(GridPoint)++;
        }

        // Assign guards to patrol high-incursion areas
        int32 GuardIndex = 0;
        for (auto& HeatPoint : IncursionHeatMap) {
            if (GuardIndex >= RegisteredAI.Num())
                break;

            if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(RegisteredAI[GuardIndex])) {
                PatrolAI->SetPatrolCenter(HeatPoint.Key);
                GuardIndex++;
            }
        }
    }
}

TArray<FTacticalInsight> UAIOverlordManager::GenerateTacticalInsights()
{
    TArray<FTacticalInsight> Insights;

    // Analyze player movement patterns
    if (RecentPlayerPositions.Num() > 10) {
        FTacticalInsight PlayerPatternInsight;
        PlayerPatternInsight.InsightType = "PlayerMovementPattern";
        PlayerPatternInsight.Priority = 2.0f;

        FVector AveragePos = FVector::ZeroVector;
        for (const FVector& Pos : RecentPlayerPositions) {
            AveragePos += Pos;
        }
        AveragePos /= RecentPlayerPositions.Num();
        PlayerPatternInsight.TargetLocation = AveragePos;

        Insights.Add(PlayerPatternInsight);
    }

    // Analyze weak patrol areas
    if (CurrentAnalysisData.GuardDeathLocations.Num() > 2) {
        FTacticalInsight WeakAreaInsight;
        WeakAreaInsight.InsightType = "WeakPatrolArea";
        WeakAreaInsight.Priority = 3.0f;

        FVector AverageDeathLocation = FVector::ZeroVector;
        for (const FVector& Death : CurrentAnalysisData.GuardDeathLocations) {
            AverageDeathLocation += Death;
        }
        AverageDeathLocation /= CurrentAnalysisData.GuardDeathLocations.Num();
        WeakAreaInsight.TargetLocation = AverageDeathLocation;

        Insights.Add(WeakAreaInsight);
    }

    return Insights;
}

void UAIOverlordManager::UpdateAIIntelligence(float DeltaTime)
{
    PerformRealTimeAnalysis();
}

void UAIOverlordManager::AdaptToPlayerBehavior()
{
    AnalyzePlayerBehaviorPatterns();

    // Adjust AI behavior based on player patterns
    if (RecentPlayerPositions.Num() > 20) {
        // Calculate player's preferred approach routes
        TArray<FVector> ApproachVectors;
        for (int32 i = 1; i < RecentPlayerPositions.Num(); i++) {
            FVector Movement = RecentPlayerPositions[i] - RecentPlayerPositions[i - 1];
            if (Movement.Size() > 100.0f) { // Filter out small movements
                ApproachVectors.Add(Movement.GetSafeNormal());
            }
        }

        // Adapt patrol positions to counter player routes
        IssueGlobalCommand("AdaptToPlayerRoutes", ApproachVectors);
    }
}

void UAIOverlordManager::IssueGlobalCommand(const FString& Command, const TArray<FVector>& Parameters)
{
    CleanupInvalidAI();

    for (AACFAIController* AI : RegisteredAI) {
        if (AI && IsValid(AI)) {
            if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AI)) {
                PatrolAI->ReceiveOverlordCommand(Command, Parameters);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AI Overlord: Issued global command '%s' to %d guards"), *Command, RegisteredAI.Num());
}

void UAIOverlordManager::IssueSelectiveCommand(const FString& Command, int32 MaxUnits, const TArray<FVector>& Parameters)
{
    CleanupInvalidAI();

    int32 UnitsCommandedCount = 0;
    for (AACFAIController* AI : RegisteredAI) {
        if (UnitsCommandedCount >= MaxUnits)
            break;

        if (AI && IsValid(AI)) {
            if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AI)) {
                PatrolAI->ReceiveOverlordCommand(Command, Parameters);
                UnitsCommandedCount++;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AI Overlord: Issued selective command '%s' to %d/%d guards"), *Command, UnitsCommandedCount, MaxUnits);
}

void UAIOverlordManager::AlertNearbyGuards(FVector AlertLocation, float AlertRadius)
{
    CleanupInvalidAI();

    TArray<FVector> AlertParameters = { AlertLocation };

    for (AACFAIController* AI : RegisteredAI) {
        if (AI && IsValid(AI) && AI->GetPawn()) {
            float Distance = FVector::Dist(AI->GetPawn()->GetActorLocation(), AlertLocation);

            if (Distance <= AlertRadius) {
                if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AI)) {
                    PatrolAI->ReceiveOverlordCommand("InvestigateAlert", AlertParameters);
                }

                if (AlertCommandTag.IsValid()) {
                    SendACFCommand(AI, AlertCommandTag);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AI Overlord: Alerted guards within %.1f units of %s"), AlertRadius, *AlertLocation.ToString());
}

void UAIOverlordManager::StartContinuousAnalysis()
{
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(AnalysisTimer, this, &UAIOverlordManager::OnAnalysisTimer, AnalysisInterval, true);
        World->GetTimerManager().SetTimer(PlayerTrackingTimer, this, &UAIOverlordManager::OnPlayerTrackingTimer, PlayerTrackingInterval, true);
    }
}

void UAIOverlordManager::PerformRealTimeAnalysis()
{
    TArray<FTacticalInsight> Insights = GenerateTacticalInsights();

    for (const FTacticalInsight& Insight : Insights) {
        if (Insight.InsightType == "PlayerMovementPattern") {
            AdaptToPlayerBehavior();
        } else if (Insight.InsightType == "WeakPatrolArea") {
            IssueSelectiveCommand("ReinforceArea", RegisteredAI.Num() / 3, { Insight.TargetLocation });
        }
    }
}

void UAIOverlordManager::TrackPlayerMovement()
{
    if (UWorld* World = GetWorld()) {
        TArray<AActor*> PlayerPawns;
        UGameplayStatics::GetAllActorsOfClass(World, APawn::StaticClass(), PlayerPawns);

        for (AActor* Actor : PlayerPawns) {
            if (APawn* Pawn = Cast<APawn>(Actor)) {
                if (Pawn->IsPlayerControlled()) {
                    RecordPlayerPosition(Pawn->GetActorLocation());
                }
            }
        }
    }
}

FACFAIUpgradeData UAIOverlordManager::CalculateAIUpgrades(float IntelligenceLevel)
{
    FACFAIUpgradeData UpgradeData;

    UpgradeData.MovementSpeedMultiplier = 1.0f + (IntelligenceLevel - 1.0f) * 0.15f;
    UpgradeData.PatrolRadiusMultiplier = 1.0f + (IntelligenceLevel - 1.0f) * 0.1f;
    UpgradeData.DetectionRangeMultiplier = 1.0f + (IntelligenceLevel - 1.0f) * 0.2f;
    UpgradeData.AccuracyMultiplier = 1.0f + (IntelligenceLevel - 1.0f) * 0.15f;
    UpgradeData.AggressionLevel = FMath::Min(IntelligenceLevel, 3.0f);
    UpgradeData.bEnableAdvancedTactics = IntelligenceLevel >= 2.0f;
    UpgradeData.bCanCoordinate = IntelligenceLevel >= 3.0f;
    UpgradeData.ResponseTime = FMath::Max(0.2f, 1.0f - (IntelligenceLevel - 1.0f) * 0.15f);

    return UpgradeData;
}

void UAIOverlordManager::FindPortalTarget()
{
    if (UWorld* World = GetWorld()) {
        PortalTarget = Cast<APortalCore>(UGameplayStatics::GetActorOfClass(World, APortalCore::StaticClass()));

        if (PortalTarget) {
            UE_LOG(LogTemp, Log, TEXT("AI Overlord: Found portal to defend"));
        }
    }
}

void UAIOverlordManager::CleanupInvalidAI()
{
    RegisteredAI.RemoveAll([](const TObjectPtr<AACFAIController>& AI) {
        return !AI || !IsValid(AI) || !AI->GetPawn() || !IsValid(AI->GetPawn());
    });
}

void UAIOverlordManager::AnalyzePlayerBehaviorPatterns()
{
    if (RecentPlayerPositions.Num() < 5) {
        return;
    }

    // Analyze movement speed patterns
    TArray<float> MovementSpeeds;
    for (int32 i = 1; i < RecentPlayerPositions.Num(); i++) {
        float Distance = FVector::Dist(RecentPlayerPositions[i], RecentPlayerPositions[i - 1]);
        MovementSpeeds.Add(Distance);
    }

    // Adjust AI response based on player behavior patterns
    if (MovementSpeeds.Num() > 0) {
        float AverageSpeed = 0.0f;
        for (float Speed : MovementSpeeds) {
            AverageSpeed += Speed;
        }
        AverageSpeed /= MovementSpeeds.Num();

        // If player moves fast, increase AI detection range
        if (AverageSpeed > 500.0f) {
            IssueGlobalCommand("IncreaseDetectionRange", TArray<FVector>());
        }
    }
}

void UAIOverlordManager::SetACFPatrolBehavior(AACFAIController* AIController, const FACFAIUpgradeData& UpgradeData)
{
    if (!AIController)
        return;

    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AIController)) {
        FPortalAIData NewAIData = PatrolAI->GetCurrentAIData();
        NewAIData.MovementSpeed *= UpgradeData.MovementSpeedMultiplier;
        NewAIData.PatrolRadius *= UpgradeData.PatrolRadiusMultiplier;
        NewAIData.PlayerDetectionRange *= UpgradeData.DetectionRangeMultiplier;
        NewAIData.AccuracyMultiplier = UpgradeData.AccuracyMultiplier;
        NewAIData.AggressionLevel = UpgradeData.AggressionLevel;
        NewAIData.bCanFlank = UpgradeData.bEnableAdvancedTactics;
        NewAIData.ReactionTime = UpgradeData.ResponseTime;

        PatrolAI->ApplyAIUpgrade(NewAIData);
    }
}

void UAIOverlordManager::SendACFCommand(AACFAIController* AIController, const FGameplayTag& CommandTag)
{
    if (!AIController || !CommandTag.IsValid())
        return;

    if (UACFCommandsManagerComponent* CommandManager = AIController->GetCommandManager()) {
        CommandManager->TriggerCommand(CommandTag);
    }
}

void UAIOverlordManager::OnAnalysisTimer()
{
    AnalyzePatrolPerformance();
    PerformRealTimeAnalysis();
}

void UAIOverlordManager::OnPlayerTrackingTimer()
{
    TrackPlayerMovement();
}