// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AILODManager.h"
#include "Algo/Sort.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "TimerManager.h"

TObjectPtr<UAILODManager> UAILODManager::InstancePtr = nullptr;

UAILODManager::UAILODManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;

    // Default LOD settings optimized for 200 AI
    LODSettings.InactiveDistance = 5000.0f;
    LODSettings.MinimalDistance = 3500.0f;
    LODSettings.StandardDistance = 2000.0f;
    LODSettings.HighDistance = 1000.0f;
    LODSettings.MaximumDistance = 500.0f;
    LODSettings.MaxHighLODAI = 15;
    LODSettings.MaxMaximumLODAI = 8;
    LODSettings.LODUpdateFrequency = 0.5f;
    LODSettings.bUsePlayerPredictiveLOD = true;
    LODSettings.PredictionRadius = 1500.0f;

    AverageFrameTime = 16.67f;
    PredictedPlayerPosition = FVector::ZeroVector;
    LastPlayerPosition = FVector::ZeroVector;
}

void UAILODManager::BeginPlay()
{
    Super::BeginPlay();

    InstancePtr = this;

    UpdatePlayerReference();
    StartLODUpdateTimer();

    // Start performance monitoring
    GetWorld()->GetTimerManager().SetTimer(PerformanceMonitorTimer, this,
        &UAILODManager::MonitorPerformance, 1.0f, true);

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager initialized - Target: 200+ AI support"));
}

void UAILODManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopLODUpdateTimer();

    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(PerformanceMonitorTimer);
    }

    if (InstancePtr == this) {
        InstancePtr = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

UAILODManager* UAILODManager::GetInstance(UWorld* World)
{
    if (!InstancePtr && World) {
        // Try to find existing instance
        for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
            if (UAILODManager* LODManager = ActorItr->FindComponentByClass<UAILODManager>()) {
                InstancePtr = LODManager;
                break;
            }
        }

        // Create new instance if none found
        if (!InstancePtr) {
            if (AGameModeBase* GameMode = World->GetAuthGameMode()) {
                InstancePtr = NewObject<UAILODManager>(GameMode);
                GameMode->AddInstanceComponent(InstancePtr);
                InstancePtr->RegisterComponent();
            }
        }
    }

    return InstancePtr;
}

void UAILODManager::RegisterAI(AACFAIController* AIController)
{
    if (!AIController || !IsValid(AIController)) {
        return;
    }

    // Check if already registered
    for (const FAILODData& Data : RegisteredAI) {
        if (Data.AIController == AIController) {
            return; // Already registered
        }
    }

    // Create new LOD data entry
    FAILODData NewData;
    NewData.AIController = AIController;
    NewData.CurrentLODLevel = EAILODLevel::Standard;
    NewData.DistanceToPlayer = 9999.0f;
    NewData.LODPriority = 1.0f;
    NewData.bInCombat = false;
    NewData.bIsEngagingPlayer = false;
    NewData.LastLODUpdateTime = GetWorld()->GetTimeSeconds();

    RegisteredAI.Add(NewData);

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Registered AI %s (Total: %d)"),
        *AIController->GetName(), RegisteredAI.Num());
}

void UAILODManager::UnregisterAI(AACFAIController* AIController)
{
    if (!AIController) {
        return;
    }

    RegisteredAI.RemoveAll([AIController](const FAILODData& Data) {
        return Data.AIController == AIController;
    });

    // Remove from forced LOD timers
    ForcedLODTimers.Remove(AIController);

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Unregistered AI %s (Total: %d)"),
        *AIController->GetName(), RegisteredAI.Num());
}

void UAILODManager::UpdateAILOD()
{
    if (RegisteredAI.Num() == 0) {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // Update player position prediction
    PredictPlayerPosition();

    // Track LOD level counts
    TMap<EAILODLevel, int32> LODCounts;
    LODCounts.Add(EAILODLevel::Inactive, 0);
    LODCounts.Add(EAILODLevel::Minimal, 0);
    LODCounts.Add(EAILODLevel::Standard, 0);
    LODCounts.Add(EAILODLevel::High, 0);
    LODCounts.Add(EAILODLevel::Maximum, 0);

    // Update all AI LOD data
    for (FAILODData& Data : RegisteredAI) {
        if (!Data.AIController || !IsValid(Data.AIController)) {
            continue;
        }

        UpdateAILODData(Data);

        // Calculate new LOD level
        EAILODLevel NewLODLevel = CalculateAILODLevel(Data);

        // Apply constraints for high-performance LOD levels
        if (NewLODLevel == EAILODLevel::Maximum && LODCounts[EAILODLevel::Maximum] >= LODSettings.MaxMaximumLODAI) {
            NewLODLevel = EAILODLevel::High;
        }

        if (NewLODLevel == EAILODLevel::High && LODCounts[EAILODLevel::High] >= LODSettings.MaxHighLODAI) {
            NewLODLevel = EAILODLevel::Standard;
        }

        // Check for forced LOD override
        if (ForcedLODTimers.Contains(Data.AIController)) {
            float* Timer = ForcedLODTimers.Find(Data.AIController);
            if (Timer && *Timer > 0.0f) {
                // Keep current forced LOD level
                NewLODLevel = Data.CurrentLODLevel;
            }
        }

        // Update LOD level if changed
        if (Data.CurrentLODLevel != NewLODLevel) {
            EAILODLevel PreviousLOD = Data.CurrentLODLevel;
            Data.CurrentLODLevel = NewLODLevel;
            Data.LastLODUpdateTime = CurrentTime;

            // Broadcast LOD change event
            OnAILODChanged.Broadcast(Data.AIController, NewLODLevel);

            UE_LOG(LogTemp, VeryVerbose, TEXT("AI LOD Manager: %s LOD changed from %d to %d"),
                *Data.AIController->GetName(),
                static_cast<int32>(PreviousLOD),
                static_cast<int32>(NewLODLevel));
        }

        // Update count
        LODCounts[Data.CurrentLODLevel]++;
    }

    // Process forced LOD timers
    ProcessForcedLODTimers();

    // Log performance metrics
    UE_LOG(LogTemp, VeryVerbose, TEXT("AI LOD Distribution - Inactive: %d, Minimal: %d, Standard: %d, High: %d, Maximum: %d"),
        LODCounts[EAILODLevel::Inactive],
        LODCounts[EAILODLevel::Minimal],
        LODCounts[EAILODLevel::Standard],
        LODCounts[EAILODLevel::High],
        LODCounts[EAILODLevel::Maximum]);
}

void UAILODManager::SetAILODLevel(AACFAIController* AIController, EAILODLevel NewLODLevel)
{
    if (!AIController) {
        return;
    }

    for (FAILODData& Data : RegisteredAI) {
        if (Data.AIController == AIController) {
            if (Data.CurrentLODLevel != NewLODLevel) {
                EAILODLevel PreviousLOD = Data.CurrentLODLevel;
                Data.CurrentLODLevel = NewLODLevel;
                Data.LastLODUpdateTime = GetWorld()->GetTimeSeconds();

                OnAILODChanged.Broadcast(AIController, NewLODLevel);

                UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Manually set %s LOD from %d to %d"),
                    *AIController->GetName(),
                    static_cast<int32>(PreviousLOD),
                    static_cast<int32>(NewLODLevel));
            }
            break;
        }
    }
}

void UAILODManager::ForceHighLOD(AACFAIController* AIController, float Duration)
{
    if (!AIController || Duration <= 0.0f) {
        return;
    }

    SetAILODLevel(AIController, EAILODLevel::High);
    ForcedLODTimers.Add(AIController, Duration);

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Forced High LOD for %s (Duration: %.1fs)"),
        *AIController->GetName(), Duration);
}

void UAILODManager::ForceMaximumLOD(AACFAIController* AIController, float Duration)
{
    if (!AIController || Duration <= 0.0f) {
        return;
    }

    SetAILODLevel(AIController, EAILODLevel::Maximum);
    ForcedLODTimers.Add(AIController, Duration);

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Forced Maximum LOD for %s (Duration: %.1fs)"),
        *AIController->GetName(), Duration);
}

int32 UAILODManager::GetAICountByLOD(EAILODLevel LODLevel) const
{
    int32 Count = 0;
    for (const FAILODData& Data : RegisteredAI) {
        if (Data.CurrentLODLevel == LODLevel) {
            Count++;
        }
    }
    return Count;
}

void UAILODManager::StartLODUpdateTimer()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(LODUpdateTimer, this,
            &UAILODManager::OnLODUpdateTimer, LODSettings.LODUpdateFrequency, true);
    }
}

void UAILODManager::StopLODUpdateTimer()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(LODUpdateTimer);
    }
}

void UAILODManager::OnLODUpdateTimer()
{
    UpdateAILOD();
}

void UAILODManager::MonitorPerformance()
{
    if (GetWorld()) {
        const float CurrentFPS = 1.0f / GetWorld()->GetDeltaSeconds();
        const float CurrentFrameTime = GetWorld()->GetDeltaSeconds() * 1000.0f; // Convert to milliseconds

        FrameTimes.Add(CurrentFrameTime);
        if (FrameTimes.Num() > 60) { // Keep last 60 frames
            FrameTimes.RemoveAt(0);
        }

        // Calculate average frame time
        float TotalFrameTime = 0.0f;
        for (float FrameTime : FrameTimes) {
            TotalFrameTime += FrameTime;
        }
        AverageFrameTime = FrameTimes.Num() > 0 ? TotalFrameTime / FrameTimes.Num() : 16.67f;

        // Adjust LOD update frequency based on performance
        if (AverageFrameTime > 33.33f) { // Below 30 FPS
            LODSettings.LODUpdateFrequency = FMath::Max(LODSettings.LODUpdateFrequency * 1.1f, 1.0f);
        } else if (AverageFrameTime < 16.67f) { // Above 60 FPS
            LODSettings.LODUpdateFrequency = FMath::Min(LODSettings.LODUpdateFrequency * 0.9f, 0.1f);
        }
    }
}

void UAILODManager::UpdatePlayerReference()
{
    if (GetWorld()) {
        if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController()) {
            if (APawn* PlayerPawn = PlayerController->GetPawn()) {
                LastPlayerPosition = PlayerPawn->GetActorLocation();
            }
        }
    }
}

void UAILODManager::PredictPlayerPosition()
{
    if (!GetWorld()) {
        return;
    }

    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController()) {
        if (APawn* PlayerPawn = PlayerController->GetPawn()) {
            FVector CurrentPlayerPosition = PlayerPawn->GetActorLocation();

            if (LODSettings.bUsePlayerPredictiveLOD) {
                FVector PlayerVelocity = (CurrentPlayerPosition - LastPlayerPosition) / LODSettings.LODUpdateFrequency;
                PredictedPlayerPosition = CurrentPlayerPosition + (PlayerVelocity * LODSettings.LODUpdateFrequency * 2.0f);
            } else {
                PredictedPlayerPosition = CurrentPlayerPosition;
            }

            LastPlayerPosition = CurrentPlayerPosition;
        }
    }
}

EAILODLevel UAILODManager::CalculateAILODLevel(const FAILODData& AIData) const
{
    // Combat state takes priority
    if (AIData.bInCombat || AIData.bIsEngagingPlayer) {
        return AIData.DistanceToPlayer <= LODSettings.MaximumDistance ? EAILODLevel::Maximum : EAILODLevel::High;
    }

    // Distance-based LOD calculation using predicted player position
    FVector ComparisonPosition = LODSettings.bUsePlayerPredictiveLOD ? PredictedPlayerPosition : LastPlayerPosition;

    if (AIData.AIController && AIData.AIController->GetPawn()) {
        float DistanceToPosition = FVector::Dist(AIData.AIController->GetPawn()->GetActorLocation(), ComparisonPosition);

        if (DistanceToPosition <= LODSettings.MaximumDistance) {
            return EAILODLevel::Maximum;
        } else if (DistanceToPosition <= LODSettings.HighDistance) {
            return EAILODLevel::High;
        } else if (DistanceToPosition <= LODSettings.StandardDistance) {
            return EAILODLevel::Standard;
        } else if (DistanceToPosition <= LODSettings.MinimalDistance) {
            return EAILODLevel::Minimal;
        }
    }

    return EAILODLevel::Inactive;
}

void UAILODManager::UpdateAILODData(FAILODData& AIData)
{
    if (!AIData.AIController || !IsValid(AIData.AIController)) {
        return;
    }

    // Update distance to player
    if (AIData.AIController->GetPawn()) {
        FVector ComparisonPosition = LODSettings.bUsePlayerPredictiveLOD ? PredictedPlayerPosition : LastPlayerPosition;
        AIData.DistanceToPlayer = FVector::Dist(AIData.AIController->GetPawn()->GetActorLocation(), ComparisonPosition);
    }

    // Update combat status for Portal Defense AI Controllers
    if (APortalDefenseAIController* PortalAI = Cast<APortalDefenseAIController>(AIData.AIController)) {
        AIData.bInCombat = PortalAI->IsInCombat();
        AIData.bIsEngagingPlayer = PortalAI->IsEngagingPlayer();
    }

    // Calculate LOD priority based on multiple factors
    float Priority = 1.0f;

    // Combat priority boost
    if (AIData.bInCombat)
        Priority += 2.0f;
    if (AIData.bIsEngagingPlayer)
        Priority += 3.0f;

    // Distance-based priority
    Priority += FMath::Max(0.0f, (LODSettings.StandardDistance - AIData.DistanceToPlayer) / LODSettings.StandardDistance);

    AIData.LODPriority = Priority;
}

void UAILODManager::ProcessForcedLODTimers()
{
    const float DeltaTime = LODSettings.LODUpdateFrequency;

    TArray<TObjectPtr<AACFAIController>> ExpiredControllers;

    for (auto& TimerPair : ForcedLODTimers) {
        TimerPair.Value -= DeltaTime;

        if (TimerPair.Value <= 0.0f) {
            ExpiredControllers.Add(TimerPair.Key);
        }
    }

    // Remove expired timers
    for (const TObjectPtr<AACFAIController>& Controller : ExpiredControllers) {
        ForcedLODTimers.Remove(Controller);

        UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Forced LOD expired for %s"),
            Controller ? *Controller->GetName() : TEXT("NULL"));
    }
}