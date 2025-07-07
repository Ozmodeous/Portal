// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AILODManager.h"
#include "AIBatchProcessor.h"
#include "Components/CombatBehaviourComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "Misc/DateTime.h"
#include "PortalDefenseAIController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAILODManager, Log, All);

// Static instance pointer for singleton access
TObjectPtr<UAILODManager> UAILODManager::InstancePtr = nullptr;

UAILODManager::UAILODManager()
{
    // Set this component to be ticked every frame for real-time LOD management
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // Initialize default configuration
    LODConfig = FAILODConfig();
    bAutoDetectPlayer = true;
    LODUpdateInterval = 0.5f;
    bEnablePerformanceMonitoring = true;
    PerformanceAdjustmentSensitivity = 0.5f;

    // Initialize performance metrics
    PerformanceMetrics = FAIPerformanceMetrics();

    // Initialize frame time sampling
    FrameTimeSamples.SetNum(MaxFrameSamples);
    for (int32 i = 0; i < MaxFrameSamples; ++i) {
        FrameTimeSamples[i] = 16.67f; // Default to 60 FPS
    }
    FrameSampleIndex = 0;

    // Set singleton instance
    InstancePtr = this;
}

void UAILODManager::BeginPlay()
{
    Super::BeginPlay();

    // Initialize the LOD management system
    InitializeLODSystem();

    UE_LOG(LogAILODManager, Log, TEXT("AI LOD Manager initialized with %d LOD levels"), LODConfig.LODDistances.Num());
}

void UAILODManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear the update timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(LODUpdateTimer);
    }

    // Clear all registered AI controllers
    RegisteredAIControllers.Empty();
    AIControllerLODLevels.Empty();

    // Clear singleton instance
    if (InstancePtr == this) {
        InstancePtr = nullptr;
    }

    UE_LOG(LogAILODManager, Log, TEXT("AI LOD Manager cleanup completed"));

    Super::EndPlay(EndPlayReason);
}

void UAILODManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update frame rate monitoring
    UpdateFrameRateMonitoring(DeltaTime);

    // Update performance metrics if enabled
    if (bEnablePerformanceMonitoring) {
        UpdatePerformanceMetrics(DeltaTime);
    }

    // Auto-detect player if needed
    if (bAutoDetectPlayer && !IsValid(PlayerCharacter)) {
        AutoDetectPlayerCharacter();
    }

    // Clean up invalid AI controller references periodically
    static float CleanupTimer = 0.0f;
    CleanupTimer += DeltaTime;
    if (CleanupTimer >= 5.0f) // Clean up every 5 seconds
    {
        CleanupInvalidAIControllers();
        CleanupTimer = 0.0f;
    }
}

void UAILODManager::InitializeLODSystem()
{
    // Validate LOD configuration
    if (LODConfig.LODDistances.Num() == 0) {
        UE_LOG(LogAILODManager, Error, TEXT("LOD configuration is invalid - no distance thresholds defined"));
        return;
    }

    // Ensure all LOD arrays have consistent sizes
    int32 LODLevels = LODConfig.LODDistances.Num();
    LODConfig.LODUpdateFrequencies.SetNum(LODLevels);
    LODConfig.LODBehaviorComplexity.SetNum(LODLevels);
    LODConfig.MaxAIPerLOD.SetNum(LODLevels);
    PerformanceMetrics.AICountPerLOD.SetNum(LODLevels);
    PerformanceMetrics.ProcessingTimePerLOD.SetNum(LODLevels);

    // Initialize performance metrics arrays
    for (int32 i = 0; i < LODLevels; ++i) {
        PerformanceMetrics.AICountPerLOD[i] = 0;
        PerformanceMetrics.ProcessingTimePerLOD[i] = 0.0f;
    }

    // Find the AI Batch Processor for integration
    if (UWorld* World = GetWorld()) {
        if (AGameStateBase* GameState = World->GetGameState()) {
            BatchProcessor = GameState->FindComponentByClass<UAIBatchProcessor>();
            if (!BatchProcessor) {
                UE_LOG(LogAILODManager, Warning, TEXT("AI Batch Processor not found - LOD management may be suboptimal"));
            }
        }
    }

    // Start the LOD update timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(
            LODUpdateTimer,
            this,
            &UAILODManager::UpdateLODLevels,
            LODUpdateInterval,
            true);
    }

    // Auto-detect player character if enabled
    if (bAutoDetectPlayer) {
        AutoDetectPlayerCharacter();
    }

    UE_LOG(LogAILODManager, Log, TEXT("LOD system initialized with %d levels, update interval: %.2fs"),
        LODLevels, LODUpdateInterval);
}

void UAILODManager::RegisterAIController(APortalDefenseAIController* AIController)
{
    if (!IsValid(AIController)) {
        UE_LOG(LogAILODManager, Warning, TEXT("Attempted to register invalid AI controller"));
        return;
    }

    // Check if already registered
    if (RegisteredAIControllers.Contains(AIController)) {
        UE_LOG(LogAILODManager, VeryVerbose, TEXT("AI controller %s is already registered"), *AIController->GetName());
        return;
    }

    // Add to registry
    RegisteredAIControllers.Add(AIController);

    // Calculate initial LOD level
    int32 InitialLOD = CalculateLODLevel(AIController);
    AIControllerLODLevels.Add(AIController, InitialLOD);

    // Apply LOD settings
    ApplyLODToAI(AIController, InitialLOD);

    // Register with batch processor if available
    if (IsValid(BatchProcessor)) {
        BatchProcessor->AssignAIToBatch(AIController, InitialLOD);
    }

    UE_LOG(LogAILODManager, VeryVerbose, TEXT("Registered AI controller %s with LOD level %d"),
        *AIController->GetName(), InitialLOD);

    // Update performance metrics
    PerformanceMetrics.TotalAICount = RegisteredAIControllers.Num();
    if (InitialLOD < PerformanceMetrics.AICountPerLOD.Num()) {
        PerformanceMetrics.AICountPerLOD[InitialLOD]++;
    }
}

void UAILODManager::UnregisterAIController(APortalDefenseAIController* AIController)
{
    if (!IsValid(AIController)) {
        return;
    }

    // Remove from batch processor
    if (IsValid(BatchProcessor)) {
        BatchProcessor->RemoveAIFromBatch(AIController);
    }

    // Update performance metrics before removal
    if (int32* LODLevel = AIControllerLODLevels.Find(AIController)) {
        if (*LODLevel < PerformanceMetrics.AICountPerLOD.Num()) {
            PerformanceMetrics.AICountPerLOD[*LODLevel] = FMath::Max(0, PerformanceMetrics.AICountPerLOD[*LODLevel] - 1);
        }
    }

    // Remove from registry
    RegisteredAIControllers.Remove(AIController);
    AIControllerLODLevels.Remove(AIController);

    // Update total count
    PerformanceMetrics.TotalAICount = RegisteredAIControllers.Num();

    UE_LOG(LogAILODManager, VeryVerbose, TEXT("Unregistered AI controller %s"),
        IsValid(AIController) ? *AIController->GetName() : TEXT("Invalid"));
}

int32 UAILODManager::GetAILODLevel(APortalDefenseAIController* AIController) const
{
    if (const int32* LODLevel = AIControllerLODLevels.Find(AIController)) {
        return *LODLevel;
    }
    return 1; // Default LOD level
}

void UAILODManager::UpdateLODLevels()
{
    if (!IsValid(PlayerCharacter)) {
        if (bAutoDetectPlayer) {
            AutoDetectPlayerCharacter();
        }

        if (!IsValid(PlayerCharacter)) {
            UE_LOG(LogAILODManager, VeryVerbose, TEXT("No player character available for LOD calculations"));
            return;
        }
    }

    // Reset LOD counts
    for (int32& Count : PerformanceMetrics.AICountPerLOD) {
        Count = 0;
    }

    // Update LOD for each registered AI controller
    for (TObjectPtr<APortalDefenseAIController> AIController : RegisteredAIControllers) {
        if (IsValid(AIController)) {
            int32 NewLODLevel = CalculateLODLevel(AIController);
            int32 CurrentLODLevel = GetAILODLevel(AIController);

            if (NewLODLevel != CurrentLODLevel) {
                // Update LOD level
                AIControllerLODLevels[AIController] = NewLODLevel;

                // Apply new LOD settings
                ApplyLODToAI(AIController, NewLODLevel);

                // Update batch processor assignment
                if (IsValid(BatchProcessor)) {
                    BatchProcessor->AssignAIToBatch(AIController, NewLODLevel);
                }

                // Broadcast LOD change event
                OnAILODChanged.Broadcast(AIController, CurrentLODLevel, NewLODLevel);

                UE_LOG(LogAILODManager, VeryVerbose, TEXT("AI controller %s LOD changed from %d to %d"),
                    *AIController->GetName(), CurrentLODLevel, NewLODLevel);
            }

            // Update LOD count
            if (NewLODLevel < PerformanceMetrics.AICountPerLOD.Num()) {
                PerformanceMetrics.AICountPerLOD[NewLODLevel]++;
            }
        }
    }

    // Optimize LOD distribution if performance adjustment is enabled
    if (LODConfig.bEnableDynamicLODAdjustment) {
        AdjustLODForPerformance();
    }

    UE_LOG(LogAILODManager, VeryVerbose, TEXT("Updated LOD levels for %d AI controllers"), RegisteredAIControllers.Num());
}

void UAILODManager::RefreshAIRegistry()
{
    UE_LOG(LogAILODManager, Log, TEXT("Refreshing AI registry"));

    // Clean up invalid controllers
    CleanupInvalidAIControllers();

    // Force update of all LOD levels
    UpdateLODLevels();

    UE_LOG(LogAILODManager, Log, TEXT("AI registry refreshed - %d controllers registered"), RegisteredAIControllers.Num());
}

TArray<APortalDefenseAIController*> UAILODManager::GetRegisteredAIControllers() const
{
    // Convert TObjectPtr array to regular pointer array for Blueprint compatibility
    TArray<APortalDefenseAIController*> Result;
    Result.Reserve(RegisteredAIControllers.Num());

    for (const TObjectPtr<APortalDefenseAIController>& Controller : RegisteredAIControllers) {
        if (IsValid(Controller)) {
            Result.Add(Controller);
        }
    }

    return Result;
}

void UAILODManager::SetPlayerCharacter(ACharacter* Player)
{
    PlayerCharacter = Player;
    UE_LOG(LogAILODManager, Log, TEXT("Player character set: %s"),
        IsValid(Player) ? *Player->GetName() : TEXT("None"));
}

int32 UAILODManager::CalculateLODLevel(APortalDefenseAIController* AIController) const
{
    if (!IsValid(AIController) || !IsValid(PlayerCharacter)) {
        return LODConfig.LODDistances.Num() - 1; // Return highest LOD (furthest) if no player
    }

    float Distance = CalculateDistanceToPlayer(AIController);

    // Find appropriate LOD level based on distance
    for (int32 i = 0; i < LODConfig.LODDistances.Num(); ++i) {
        if (Distance <= LODConfig.LODDistances[i]) {
            return i;
        }
    }

    // Return highest LOD level if distance exceeds all thresholds
    return LODConfig.LODDistances.Num() - 1;
}

float UAILODManager::CalculateDistanceToPlayer(APortalDefenseAIController* AIController) const
{
    if (!IsValid(AIController) || !IsValid(PlayerCharacter)) {
        return FLT_MAX;
    }

    APawn* AIPawn = AIController->GetPawn();
    if (!IsValid(AIPawn)) {
        return FLT_MAX;
    }

    return FVector::Dist(AIPawn->GetActorLocation(), PlayerCharacter->GetActorLocation());
}

void UAILODManager::ApplyLODToAI(APortalDefenseAIController* AIController, int32 NewLODLevel)
{
    if (!IsValid(AIController) || NewLODLevel < 0 || NewLODLevel >= LODConfig.LODDistances.Num()) {
        return;
    }

    // Apply update frequency
    float UpdateFrequency = LODConfig.LODUpdateFrequencies[NewLODLevel];
    AIController->SetAIUpdateFrequency(UpdateFrequency);

    // Apply behavior complexity
    float ComplexityMultiplier = LODConfig.LODBehaviorComplexity[NewLODLevel];
    AIController->SetBehaviorComplexity(ComplexityMultiplier);

    // Set LOD level on the AI controller for internal use
    AIController->SetCurrentLODLevel(NewLODLevel);

    UE_LOG(LogAILODManager, VeryVerbose, TEXT("Applied LOD %d to AI %s - Update: %.1fHz, Complexity: %.2f"),
        NewLODLevel, *AIController->GetName(), UpdateFrequency, ComplexityMultiplier);
}

void UAILODManager::UpdatePerformanceMetrics(float DeltaTime)
{
    // Update current frame rate
    PerformanceMetrics.CurrentFrameRate = 1.0f / DeltaTime;

    // Calculate average frame time
    float TotalFrameTime = 0.0f;
    for (const float& FrameTime : FrameTimeSamples) {
        TotalFrameTime += FrameTime;
    }
    PerformanceMetrics.AverageFrameTime = TotalFrameTime / FrameTimeSamples.Num();

    // Update total AI count
    PerformanceMetrics.TotalAICount = RegisteredAIControllers.Num();

    // Calculate AI memory usage (approximate)
    PerformanceMetrics.AIMemoryUsage = RegisteredAIControllers.Num() * 0.1f; // Rough estimate in MB

    UE_LOG(LogAILODManager, VeryVerbose, TEXT("Performance: FPS=%.1f, AI Count=%d, Memory=%.1fMB"),
        PerformanceMetrics.CurrentFrameRate, PerformanceMetrics.TotalAICount, PerformanceMetrics.AIMemoryUsage);
}

void UAILODManager::AdjustLODForPerformance()
{
    if (PerformanceMetrics.CurrentFrameRate < LODConfig.TargetFrameRate) {
        // Frame rate is below target - reduce AI processing load
        float FrameRateDeficit = LODConfig.TargetFrameRate - PerformanceMetrics.CurrentFrameRate;
        float AdjustmentFactor = (FrameRateDeficit / LODConfig.TargetFrameRate) * PerformanceAdjustmentSensitivity;

        // Increase LOD distances to reduce processing load
        for (int32 i = 0; i < LODConfig.LODDistances.Num(); ++i) {
            LODConfig.LODDistances[i] *= (1.0f + AdjustmentFactor * 0.1f);
        }

        // Reduce update frequencies
        for (int32 i = 0; i < LODConfig.LODUpdateFrequencies.Num(); ++i) {
            LODConfig.LODUpdateFrequencies[i] *= (1.0f - AdjustmentFactor * 0.2f);
            LODConfig.LODUpdateFrequencies[i] = FMath::Max(0.1f, LODConfig.LODUpdateFrequencies[i]);
        }

        OnPerformanceAdjustment.Broadcast(PerformanceMetrics.CurrentFrameRate);

        UE_LOG(LogAILODManager, Log, TEXT("Performance adjustment applied - Frame rate deficit: %.1f, Adjustment: %.3f"),
            FrameRateDeficit, AdjustmentFactor);
    }
}

void UAILODManager::AutoDetectPlayerCharacter()
{
    if (UWorld* World = GetWorld()) {
        // Try to get the first player controller's pawn
        if (APlayerController* PC = World->GetFirstPlayerController()) {
            if (ACharacter* PlayerChar = Cast<ACharacter>(PC->GetPawn())) {
                SetPlayerCharacter(PlayerChar);
                UE_LOG(LogAILODManager, Log, TEXT("Auto-detected player character: %s"), *PlayerChar->GetName());
            }
        }
    }
}

void UAILODManager::CleanupInvalidAIControllers()
{
    int32 RemovedCount = 0;

    // Remove invalid controllers from the main array
    RegisteredAIControllers.RemoveAll([&RemovedCount](const TObjectPtr<APortalDefenseAIController>& Controller) {
        bool bShouldRemove = !IsValid(Controller);
        if (bShouldRemove) {
            RemovedCount++;
        }
        return bShouldRemove;
    });

    // Clean up the LOD levels map
    TArray<TObjectPtr<APortalDefenseAIController>> InvalidControllers;
    for (const auto& Pair : AIControllerLODLevels) {
        if (!IsValid(Pair.Key)) {
            InvalidControllers.Add(Pair.Key);
        }
    }

    for (const auto& InvalidController : InvalidControllers) {
        AIControllerLODLevels.Remove(InvalidController);
    }

    if (RemovedCount > 0) {
        UE_LOG(LogAILODManager, Log, TEXT("Cleaned up %d invalid AI controller references"), RemovedCount);
        PerformanceMetrics.TotalAICount = RegisteredAIControllers.Num();
    }
}

void UAILODManager::OptimizeLODDistribution()
{
    // Calculate current distribution
    TArray<int32> CurrentDistribution = PerformanceMetrics.AICountPerLOD;

    // Find overloaded LOD levels
    for (int32 i = 0; i < CurrentDistribution.Num(); ++i) {
        if (i < LODConfig.MaxAIPerLOD.Num() && CurrentDistribution[i] > LODConfig.MaxAIPerLOD[i]) {
            // This LOD level is overloaded - try to move some AI to higher LOD
            int32 Excess = CurrentDistribution[i] - LODConfig.MaxAIPerLOD[i];

            // Slightly increase distance threshold to push some AI to next LOD level
            if (i < LODConfig.LODDistances.Num() - 1) {
                LODConfig.LODDistances[i] *= 0.95f; // Reduce by 5%
                UE_LOG(LogAILODManager, VeryVerbose, TEXT("Reduced LOD %d distance threshold to balance load"), i);
            }
        }
    }
}

void UAILODManager::UpdateFrameRateMonitoring(float DeltaTime)
{
    // Convert delta time to milliseconds
    float FrameTimeMs = DeltaTime * 1000.0f;

    // Add to circular buffer
    FrameTimeSamples[FrameSampleIndex] = FrameTimeMs;
    FrameSampleIndex = (FrameSampleIndex + 1) % MaxFrameSamples;
}

// Static Access Functions
UAILODManager* UAILODManager::GetInstance()
{
    return InstancePtr;
}

// Blueprint Accessible Getters
FAIPerformanceMetrics UAILODManager::GetPerformanceMetrics() const
{
    return PerformanceMetrics;
}

int32 UAILODManager::GetAICountAtLODLevel(int32 LODLevel) const
{
    if (LODLevel >= 0 && LODLevel < PerformanceMetrics.AICountPerLOD.Num()) {
        return PerformanceMetrics.AICountPerLOD[LODLevel];
    }
    return 0;
}

int32 UAILODManager::GetTotalAICount() const
{
    return RegisteredAIControllers.Num();
}

bool UAILODManager::IsLODSystemActive() const
{
    return LODUpdateTimer.IsValid();
}

float UAILODManager::GetAverageFrameRate() const
{
    return PerformanceMetrics.AverageFrameTime > 0.0f ? 1000.0f / PerformanceMetrics.AverageFrameTime : 0.0f;
}

FString UAILODManager::GetLODLevelName(int32 LODLevel) const
{
    switch (LODLevel) {
    case 0:
        return TEXT("Very High Detail");
    case 1:
        return TEXT("High Detail");
    case 2:
        return TEXT("Medium Detail");
    case 3:
        return TEXT("Low Detail");
    case 4:
        return TEXT("Very Low Detail");
    default:
        return FString::Printf(TEXT("LOD Level %d"), LODLevel);
    }
}