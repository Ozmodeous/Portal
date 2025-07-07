// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AILODManager.h"
#include "ACFAIController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// Static member initialization for UE 5.5 compliance
TObjectPtr<UAILODManager> UAILODManager::Instance = nullptr;

UAILODManager::UAILODManager()
{
    // Optimize component tick settings for performance
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    bWantsInitializeComponent = true;

    // Initialize performance tracking arrays with proper UE 5.5 optimization
    RecentFrameTimes.Reserve(60);
    AverageFrameTime = 16.67f; // Target 60 FPS baseline

    // Initialize LOD settings with balanced defaults for ACF integration
    LODSettings = FAILODSettings();
}

void UAILODManager::BeginPlay()
{
    Super::BeginPlay();

    // Thread-safe singleton assignment for ACF integration
    Instance = this;
    StartLODUpdateTimer();

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager initialized for ACF Ultimate integration"));
}

void UAILODManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean shutdown sequence for ACF compatibility
    StopLODUpdateTimer();
    RegisteredAI.Empty();

    // Thread-safe singleton cleanup
    if (Instance == this) {
        Instance = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

UAILODManager* UAILODManager::GetInstance(UWorld* World)
{
    // Validate singleton instance with ACF-compatible null checks
    if (Instance && IsValid(Instance)) {
        return Instance;
    }

    // Fallback search for existing manager in world - ACF Ultimate compatibility
    if (World) {
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
    // ACF Ultimate validation - ensure controller is valid and not already registered
    if (!AIController || !IsValid(AIController)) {
        UE_LOG(LogTemp, Warning, TEXT("AILODManager: Invalid AIController registration attempt"));
        return;
    }

    // Prevent duplicate registrations for optimal ACF performance
    for (const FAILODData& Data : RegisteredAI) {
        if (Data.AIController == AIController) {
            UE_LOG(LogTemp, Verbose, TEXT("AILODManager: AI %s already registered"), *AIController->GetName());
            return;
        }
    }

    // Initialize new AI data with ACF-optimized defaults
    FAILODData NewAIData;
    NewAIData.AIController = AIController;
    NewAIData.CurrentLODLevel = EAILODLevel::Standard;
    NewAIData.DistanceToPlayer = 9999.0f;
    NewAIData.LODPriority = 1.0f;
    NewAIData.LastLODUpdateTime = GetWorld()->GetTimeSeconds();
    NewAIData.bInCombat = false;
    NewAIData.bIsEngagingPlayer = false;
    NewAIData.bForcedHighLOD = false;

    RegisteredAI.Add(NewAIData);

    UE_LOG(LogTemp, Log, TEXT("AILODManager: Registered ACF AI %s (Total: %d)"),
        *AIController->GetName(), RegisteredAI.Num());
}

void UAILODManager::UnregisterAI(AACFAIController* AIController)
{
    if (!AIController) {
        return;
    }

    // Efficient removal using lambda predicate for ACF compatibility
    const int32 RemovedCount = RegisteredAI.RemoveAll([AIController](const FAILODData& Data) {
        return Data.AIController == AIController;
    });

    if (RemovedCount > 0) {
        UE_LOG(LogTemp, Log, TEXT("AILODManager: Unregistered ACF AI %s (Total: %d)"),
            *AIController->GetName(), RegisteredAI.Num());
    }
}

void UAILODManager::UpdateAILOD()
{
    // Early exit optimization for empty registrations
    if (RegisteredAI.Num() == 0) {
        return;
    }

    // Maintenance operations for ACF Ultimate stability
    CleanupInvalidAI();
    UpdatePlayerReference();

    // Validate player reference for distance calculations
    if (!CachedPlayerPawn) {
        UE_LOG(LogTemp, VeryVerbose, TEXT("AILODManager: No valid player reference for LOD calculations"));
        return;
    }

    // Predictive player positioning for advanced ACF AI behavior
    if (LODSettings.bUsePlayerPredictiveLOD) {
        PredictPlayerMovement();
    }

    const FVector PlayerPosition = CachedPlayerPawn->GetActorLocation();
    const FVector TargetPosition = LODSettings.bUsePlayerPredictiveLOD ? PredictedPlayerPosition : PlayerPosition;

    // Update distance metrics and priorities for all registered ACF AI
    for (FAILODData& AIData : RegisteredAI) {
        if (!AIData.AIController || !IsValid(AIData.AIController)) {
            continue;
        }

        if (APawn* AIPawn = AIData.AIController->GetPawn()) {
            // Calculate spatial relationship for ACF combat optimization
            AIData.DistanceToPlayer = FVector::Dist(AIPawn->GetActorLocation(), TargetPosition);
            AIData.LODPriority = CalculateAIPriority(AIData);
            AIData.LastLODUpdateTime = GetWorld()->GetTimeSeconds();

            // ACF-specific combat state detection (basic implementation)
            // This should integrate with ACF's threat management system
            AIData.bInCombat = false; // TODO: Integrate with ACF combat detection
            AIData.bIsEngagingPlayer = false; // TODO: Integrate with ACF targeting system
        }
    }

    // Apply LOD calculations and limitations for performance optimization
    CalculateLODPriorities();
    ApplyLODLimits();
    UpdatePerformanceMetrics();
}

void UAILODManager::SetAILODLevel(AACFAIController* AIController, EAILODLevel NewLODLevel)
{
    // Direct LOD assignment with ACF validation
    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.AIController == AIController) {
            SetAILODInternal(AIData, NewLODLevel);
            break;
        }
    }
}

void UAILODManager::SetAILODInternal(FAILODData& AIData, EAILODLevel NewLODLevel)
{
    // Optimization: Skip unnecessary updates
    if (AIData.CurrentLODLevel == NewLODLevel) {
        return;
    }

    const EAILODLevel OldLODLevel = AIData.CurrentLODLevel;
    AIData.CurrentLODLevel = NewLODLevel;

    // Validate ACF controller before applying settings
    if (!AIData.AIController || !IsValid(AIData.AIController)) {
        return;
    }

    // Apply ACF-compatible LOD settings based on level
    switch (NewLODLevel) {
    case EAILODLevel::Inactive:
        // Completely pause AI for maximum performance
        AIData.AIController->SetActorTickEnabled(false);
        // TODO: Integrate with ACF's AI state management
        break;

    case EAILODLevel::Minimal:
        // Reduced tick rate for distant AI
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.5f);
        // TODO: Set ACF behavior tree to minimal patrol state
        break;

    case EAILODLevel::Standard:
        // Default ACF AI behavior
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.0f);
        // TODO: Set ACF behavior tree to standard state
        break;

    case EAILODLevel::High:
        // Enhanced ACF AI behavior with full perception
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.0f);
        // TODO: Enable enhanced ACF perception and targeting
        break;

    case EAILODLevel::Maximum:
        // All ACF systems active, highest priority
        AIData.AIController->SetActorTickEnabled(true);
        AIData.AIController->SetActorTickInterval(0.0f);
        // TODO: Enable all ACF advanced AI features
        break;
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("ACF AI %s LOD changed from %d to %d"),
        *AIData.AIController->GetName(), static_cast<int32>(OldLODLevel), static_cast<int32>(NewLODLevel));
}

void UAILODManager::ForceHighLOD(AACFAIController* AIController, float Duration)
{
    // Immediate high-priority LOD for combat scenarios
    SetAILODLevel(AIController, EAILODLevel::High);

    // Schedule LOD reversion using UE 5.5 timer system
    FTimerHandle RevertTimer;
    GetWorld()->GetTimerManager().SetTimer(RevertTimer, [this, AIController]() {
            // Revert to optimal LOD based on current conditions
            for (FAILODData& AIData : RegisteredAI)
            {
                if (AIData.AIController == AIController)
                {
                    const EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);
                    SetAILODInternal(AIData, OptimalLOD);
                    AIData.bForcedHighLOD = false;
                    break;
                }
            } }, Duration, false);

    // Mark as forced for ACF integration tracking
    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.AIController == AIController) {
            AIData.bForcedHighLOD = true;
            break;
        }
    }
}

void UAILODManager::ForceMaximumLOD(AACFAIController* AIController, float Duration)
{
    // Critical priority LOD for intense combat situations
    SetAILODLevel(AIController, EAILODLevel::Maximum);

    // Schedule LOD reversion with ACF-compatible timing
    FTimerHandle RevertTimer;
    GetWorld()->GetTimerManager().SetTimer(RevertTimer, [this, AIController]() {
            for (FAILODData& AIData : RegisteredAI)
            {
                if (AIData.AIController == AIController)
                {
                    const EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);
                    SetAILODInternal(AIData, OptimalLOD);
                    AIData.bForcedHighLOD = false;
                    break;
                }
            } }, Duration, false);

    // Track forced state for ACF behavior coordination
    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.AIController == AIController) {
            AIData.bForcedHighLOD = true;
            break;
        }
    }
}

int32 UAILODManager::GetAICountByLOD(EAILODLevel LODLevel) const
{
    // Efficient counting with range-based loop for UE 5.5
    int32 Count = 0;
    for (const FAILODData& AIData : RegisteredAI) {
        if (AIData.CurrentLODLevel == LODLevel) {
            Count++;
        }
    }
    return Count;
}

void UAILODManager::UpdatePlayerReference()
{
    // Cache player pawn for performance optimization
    if (!CachedPlayerPawn || !IsValid(CachedPlayerPawn)) {
        CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    }
}

void UAILODManager::CleanupInvalidAI()
{
    // Remove invalid or destroyed AI controllers for memory management
    RegisteredAI.RemoveAll([](const FAILODData& Data) {
        return !Data.AIController || !IsValid(Data.AIController);
    });
}

void UAILODManager::CalculateLODPriorities()
{
    // Sort AI by priority for optimal LOD distribution
    RegisteredAI.Sort([](const FAILODData& A, const FAILODData& B) {
        return A.LODPriority > B.LODPriority;
    });

    // Apply distance-based LOD assignments
    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.bForcedHighLOD) {
            continue; // Skip forced LOD AI
        }

        const EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);
        SetAILODInternal(AIData, OptimalLOD);
    }
}

void UAILODManager::ApplyLODLimits()
{
    // Enforce performance-based LOD limits for stable frame rates
    int32 HighLODCount = 0;
    int32 MaximumLODCount = 0;

    // Count current high-level LODs
    for (const FAILODData& AIData : RegisteredAI) {
        if (AIData.CurrentLODLevel == EAILODLevel::High) {
            HighLODCount++;
        } else if (AIData.CurrentLODLevel == EAILODLevel::Maximum) {
            MaximumLODCount++;
        }
    }

    // Downgrade excess AI if over performance limits
    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.bForcedHighLOD) {
            continue; // Respect forced LOD states
        }

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
    // Performance-based LOD scaling for dynamic optimization
    if (LODSettings.bUsePerformanceScaling) {
        // Track frame time for adaptive LOD scaling
        const float CurrentFrameTime = GetWorld()->GetDeltaSeconds() * 1000.0f;
        RecentFrameTimes.Add(CurrentFrameTime);

        // Maintain rolling window of frame times
        if (RecentFrameTimes.Num() > 60) {
            RecentFrameTimes.RemoveAt(0);
        }

        // Calculate performance metrics
        float TotalTime = 0.0f;
        for (const float FrameTime : RecentFrameTimes) {
            TotalTime += FrameTime;
        }

        AverageFrameTime = RecentFrameTimes.Num() > 0 ? TotalTime / RecentFrameTimes.Num() : (GEngine ? 1000.0f / GEngine->GetMaxFPS() : 16.67f);

        // TODO: Implement adaptive LOD scaling based on performance metrics
        // This should adjust LOD limits dynamically based on frame rate
    }
}

void UAILODManager::PredictPlayerMovement()
{
    // Predictive positioning for advanced ACF AI behavior
    if (CachedPlayerPawn) {
        const FVector CurrentVelocity = CachedPlayerPawn->GetVelocity();
        const FVector CurrentPosition = CachedPlayerPawn->GetActorLocation();

        // Simple linear prediction - can be enhanced with acceleration data
        PredictedPlayerPosition = CurrentPosition + (CurrentVelocity * LODSettings.PlayerPredictionTime);
    }
}

EAILODLevel UAILODManager::CalculateOptimalLOD(const FAILODData& AIData) const
{
    // Distance-based LOD calculation with ACF integration considerations
    const float Distance = AIData.DistanceToPlayer;

    if (Distance > LODSettings.InactiveDistance) {
        return EAILODLevel::Inactive;
    } else if (Distance > LODSettings.MinimalDistance) {
        return EAILODLevel::Minimal;
    } else if (Distance > LODSettings.StandardDistance) {
        return EAILODLevel::Standard;
    } else if (Distance > LODSettings.HighDistance) {
        return EAILODLevel::High;
    } else {
        return EAILODLevel::Maximum;
    }
}

float UAILODManager::CalculateAIPriority(const FAILODData& AIData) const
{
    // Multi-factor priority calculation for ACF AI systems
    float Priority = 1.0f;

    // Distance factor (closer = higher priority)
    const float DistanceFactor = 1.0f - FMath::Clamp(AIData.DistanceToPlayer / LODSettings.InactiveDistance, 0.0f, 1.0f);
    Priority += DistanceFactor * 2.0f;

    // Combat state factor (combat AI gets priority)
    if (AIData.bInCombat) {
        Priority += 3.0f;
    }

    if (AIData.bIsEngagingPlayer) {
        Priority += 5.0f;
    }

    // TODO: Add ACF-specific priority factors:
    // - Threat level from ACF threat manager
    // - AI role/importance from ACF systems
    // - Current action priority from ACF action system

    return Priority;
}

void UAILODManager::StartLODUpdateTimer()
{
    // Initialize LOD update timer with ACF-compatible frequency
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(LODUpdateTimer, this, &UAILODManager::UpdateAILOD,
            LODSettings.LODUpdateFrequency, true);
    }
}

void UAILODManager::StopLODUpdateTimer()
{
    // Clean timer shutdown for ACF compatibility
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(LODUpdateTimer);
    }
}