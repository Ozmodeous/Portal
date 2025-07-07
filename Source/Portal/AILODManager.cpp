// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AILODManager.h"
#include "ACFAIController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

TObjectPtr<UAILODManager> UAILODManager::Instance = nullptr;

UAILODManager::UAILODManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;

    // Initialize performance tracking
    RecentFrameTimes.Reserve(60);
    AverageFrameTime = 16.67f;
}

void UAILODManager::BeginPlay()
{
    Super::BeginPlay();

    Instance = this;
    StartLODUpdateTimer();

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager initialized"));
}

void UAILODManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopLODUpdateTimer();
    RegisteredAI.Empty();
    Instance = nullptr;

    Super::EndPlay(EndPlayReason);
}

UAILODManager* UAILODManager::GetInstance(UWorld* World)
{
    if (Instance && IsValid(Instance)) {
        return Instance;
    }

    if (World) {
        // Try to find existing instance
        for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
            AActor* CurrentActor = *ActorItr;
            if (UAILODManager* FoundManager = CurrentActor->FindComponentByClass<UAILODManager>()) {
                Instance = FoundManager;
                return Instance;
            }
        }
    }

    return nullptr;
}

void UAILODManager::RegisterAI(AACFAIController* AIController)
{
    if (!AIController || !IsValid(AIController)) {
        return;
    }

    // Check if already registered
    for (const FAILODData& Data : RegisteredAI) {
        if (Data.AIController == AIController) {
            return;
        }
    }

    FAILODData NewAIData;
    NewAIData.AIController = AIController;
    NewAIData.CurrentLODLevel = EAILODLevel::Standard;
    NewAIData.DistanceToPlayer = 9999.0f;
    NewAIData.LODPriority = 1.0f;
    NewAIData.LastLODUpdateTime = GetWorld()->GetTimeSeconds();

    RegisteredAI.Add(NewAIData);

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

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Unregistered AI %s (Total: %d)"),
        *AIController->GetName(), RegisteredAI.Num());
}

void UAILODManager::UpdateAILOD()
{
    if (RegisteredAI.Num() == 0) {
        return;
    }

    CleanupInvalidAI();
    UpdatePlayerReference();

    if (!CachedPlayerPawn) {
        return;
    }

    if (LODSettings.bUsePlayerPredictiveLOD) {
        PredictPlayerMovement();
    }

    const FVector PlayerPosition = CachedPlayerPawn->GetActorLocation();
    const FVector TargetPosition = LODSettings.bUsePlayerPredictiveLOD ? PredictedPlayerPosition : PlayerPosition;

    // Update distances and priorities
    for (FAILODData& AIData : RegisteredAI) {
        if (!AIData.AIController || !IsValid(AIData.AIController))
            continue;

        if (APawn* AIPawn = AIData.AIController->GetPawn()) {
            AIData.DistanceToPlayer = FVector::Dist(AIPawn->GetActorLocation(), TargetPosition);
            AIData.LODPriority = CalculateAIPriority(AIData);
            AIData.LastLODUpdateTime = GetWorld()->GetTimeSeconds();

            // Update combat state (basic implementation)
            AIData.bInCombat = false;
            AIData.bIsEngagingPlayer = false;
        }
    }

    CalculateLODPriorities();
    ApplyLODLimits();
    UpdatePerformanceMetrics();
}

void UAILODManager::SetAILODLevel(AACFAIController* AIController, EAILODLevel NewLODLevel)
{
    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.AIController == AIController) {
            SetAILODInternal(AIData, NewLODLevel);
            break;
        }
    }
}

void UAILODManager::SetAILODInternal(FAILODData& AIData, EAILODLevel NewLODLevel)
{
    if (AIData.CurrentLODLevel == NewLODLevel)
        return;

    EAILODLevel OldLODLevel = AIData.CurrentLODLevel;
    AIData.CurrentLODLevel = NewLODLevel;

    if (!AIData.AIController || !IsValid(AIData.AIController))
        return;

    // Apply LOD settings to AI Controller
    switch (NewLODLevel) {
    case EAILODLevel::Inactive:
        AIData.AIController->SetActorTickEnabled(false);
        break;

    case EAILODLevel::Minimal:
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.5f);
        break;

    case EAILODLevel::Standard:
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.0f);
        break;

    case EAILODLevel::High:
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.0f);
        break;

    case EAILODLevel::Maximum:
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.0f);
        break;
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("AI %s LOD changed from %d to %d"),
        *AIData.AIController->GetName(), (int32)OldLODLevel, (int32)NewLODLevel);
}

void UAILODManager::ForceHighLOD(AACFAIController* AIController, float Duration)
{
    SetAILODLevel(AIController, EAILODLevel::High);

    // Set timer to revert LOD after duration
    FTimerHandle RevertTimer;
    GetWorld()->GetTimerManager().SetTimer(RevertTimer, [this, AIController]() {
        for (FAILODData& AIData : RegisteredAI)
        {
            if (AIData.AIController == AIController)
            {
                EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);
                SetAILODInternal(AIData, OptimalLOD);
                break;
            }
        } }, Duration, false);
}

void UAILODManager::ForceMaximumLOD(AACFAIController* AIController, float Duration)
{
    SetAILODLevel(AIController, EAILODLevel::Maximum);

    // Set timer to revert LOD after duration
    FTimerHandle RevertTimer;
    GetWorld()->GetTimerManager().SetTimer(RevertTimer, [this, AIController]() {
        for (FAILODData& AIData : RegisteredAI)
        {
            if (AIData.AIController == AIController)
            {
                EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);
                SetAILODInternal(AIData, OptimalLOD);
                break;
            }
        } }, Duration, false);
}

int32 UAILODManager::GetAICountByLOD(EAILODLevel LODLevel) const
{
    return RegisteredAI.FilterByPredicate([LODLevel](const FAILODData& Data) {
                           return Data.CurrentLODLevel == LODLevel;
                       })
        .Num();
}

void UAILODManager::UpdatePlayerReference()
{
    if (!CachedPlayerPawn || !IsValid(CachedPlayerPawn)) {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0)) {
            CachedPlayerPawn = PC->GetPawn();
        }
    }
}

void UAILODManager::CleanupInvalidAI()
{
    RegisteredAI.RemoveAll([](const FAILODData& Data) {
        return !Data.AIController || !IsValid(Data.AIController);
    });
}

void UAILODManager::CalculateLODPriorities()
{
    // Sort AI by priority (highest first)
    RegisteredAI.Sort([](const FAILODData& A, const FAILODData& B) {
        return A.LODPriority > B.LODPriority;
    });

    // Apply optimal LOD based on priority and distance
    for (FAILODData& AIData : RegisteredAI) {
        EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);

        if (AIData.CurrentLODLevel != OptimalLOD) {
            SetAILODInternal(AIData, OptimalLOD);
        }
    }
}

void UAILODManager::ApplyLODLimits()
{
    int32 HighLODCount = 0;
    int32 MaximumLODCount = 0;

    // First pass: Count current high-level LODs
    for (const FAILODData& AIData : RegisteredAI) {
        if (AIData.CurrentLODLevel == EAILODLevel::High)
            HighLODCount++;
        else if (AIData.CurrentLODLevel == EAILODLevel::Maximum)
            MaximumLODCount++;
    }

    // Second pass: Downgrade excess AI if over limits
    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.bForcedHighLOD)
            continue;

        if (AIData.CurrentLODLevel == EAILODLevel::Maximum && MaximumLODCount > LODSettings.MaxMaximumLODAI) {
            SetAILODInternal(AIData, EAILODLevel::High);
            MaximumLODCount--;
            HighLODCount++;
        } else if (AIData.CurrentLODLevel == EAILODLevel::High && HighLODCount > LODSettings.MaxHighLODAI) {
            SetAILODInternal(AIData, EAILODLevel::Standard);
            HighLODCount--;
        }
    }
}

void UAILODManager::UpdatePerformanceMetrics()
{
    if (LODSettings.bUsePerformanceScaling) {
        // Add current frame time
        float CurrentFrameTime = GetWorld()->GetDeltaSeconds() * 1000.0f;
        RecentFrameTimes.Add(CurrentFrameTime);

        // Keep only recent frames
        if (RecentFrameTimes.Num() > 60) {
            RecentFrameTimes.RemoveAt(0);
        }

        // Calculate average
        float TotalTime = 0.0f;
        for (float FrameTime : RecentFrameTimes) {
            TotalTime += FrameTime;
        }
        AverageFrameTime = RecentFrameTimes.Num() > 0 ? TotalTime / RecentFrameTimes.Num() : (GEngine ? 1000.0f / GEngine->GetMaxTickRate() : 16.67f);

        // Auto-adjust LOD settings based on performance
        if (AverageFrameTime > 20.0f) // Below 50 FPS
        {
            LODSettings.MaxHighLODAI = FMath::Max(5, LODSettings.MaxHighLODAI - 1);
            LODSettings.MaxMaximumLODAI = FMath::Max(3, LODSettings.MaxMaximumLODAI - 1);
        } else if (AverageFrameTime < 14.0f) // Above 70 FPS
        {
            LODSettings.MaxHighLODAI = FMath::Min(20, LODSettings.MaxHighLODAI + 1);
            LODSettings.MaxMaximumLODAI = FMath::Min(12, LODSettings.MaxMaximumLODAI + 1);
        }
    }
}

void UAILODManager::PredictPlayerMovement()
{
    if (!CachedPlayerPawn)
        return;

    FVector CurrentPosition = CachedPlayerPawn->GetActorLocation();
    FVector CurrentVelocity = CachedPlayerPawn->GetVelocity();

    // Simple prediction based on current velocity
    PredictedPlayerPosition = CurrentPosition + (CurrentVelocity * LODSettings.PlayerPredictionTime);
}

EAILODLevel UAILODManager::CalculateOptimalLOD(const FAILODData& AIData) const
{
    // Always maximum LOD for AI in combat
    if (AIData.bInCombat || AIData.bIsEngagingPlayer) {
        return EAILODLevel::Maximum;
    }

    const float Distance = AIData.DistanceToPlayer;

    if (Distance <= LODSettings.MaximumDistance) {
        return EAILODLevel::Maximum;
    } else if (Distance <= LODSettings.HighDistance) {
        return EAILODLevel::High;
    } else if (Distance <= LODSettings.StandardDistance) {
        return EAILODLevel::Standard;
    } else if (Distance <= LODSettings.MinimalDistance) {
        return EAILODLevel::Minimal;
    } else {
        return EAILODLevel::Inactive;
    }
}

float UAILODManager::CalculateAIPriority(const FAILODData& AIData) const
{
    float Priority = 1.0f;

    // Combat AI gets highest priority
    if (AIData.bInCombat) {
        Priority += 10.0f;
    }

    // AI engaging player gets very high priority
    if (AIData.bIsEngagingPlayer) {
        Priority += 8.0f;
    }

    // Distance-based priority (closer = higher)
    const float MaxDistance = LODSettings.InactiveDistance;
    const float DistanceFactor = FMath::Clamp(1.0f - (AIData.DistanceToPlayer / MaxDistance), 0.0f, 1.0f);
    Priority += DistanceFactor * 5.0f;

    return Priority;
}

void UAILODManager::StartLODUpdateTimer()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(LODUpdateTimer, this,
            &UAILODManager::UpdateAILOD, LODSettings.LODUpdateFrequency, true);
    }
}

void UAILODManager::StopLODUpdateTimer()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(LODUpdateTimer);
    }
}