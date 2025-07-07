// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AIBatchProcessor.h"
#include "AILODManager.h"
#include "Components/CombatBehaviourComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "PortalDefenseAIController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAIBatchProcessor, Log, All);

UAIBatchProcessor::UAIBatchProcessor()
{
    // Set this component to be ticked every frame for real-time batch processing
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // Initialize default batch processing settings
    MaxBatchSize = 25;
    BaseBatchProcessingInterval = 0.1f;
    bEnablePerformanceMonitoring = true;
    MaxProcessingTimePerFrame = 5.0f;
    AverageProcessingTime = 0.0f;
    ProcessedAICount = 0;

    // Configure LOD processing multipliers for performance scaling
    LODProcessingMultipliers.Add(0, 1.0f); // Highest detail - normal processing
    LODProcessingMultipliers.Add(1, 1.5f); // High detail - slightly slower processing
    LODProcessingMultipliers.Add(2, 2.0f); // Medium detail - reduced processing
    LODProcessingMultipliers.Add(3, 3.0f); // Low detail - significantly reduced processing
    LODProcessingMultipliers.Add(4, 5.0f); // Lowest detail - minimal processing

    // Initialize batch data for all priority levels
    for (int32 i = 0; i < static_cast<int32>(EAIProcessingPriority::VeryLow) + 1; ++i) {
        EAIProcessingPriority Priority = static_cast<EAIProcessingPriority>(i);
        FString BatchName = FString::Printf(TEXT("Batch_Priority_%d"), i);
        BatchData.Add(Priority, CreateNewBatch(BatchName, Priority));
    }
}

void UAIBatchProcessor::BeginPlay()
{
    Super::BeginPlay();

    // Initialize the batch processing system
    InitializeBatchProcessor();

    UE_LOG(LogAIBatchProcessor, Log, TEXT("AI Batch Processor initialized with %d priority levels"), BatchData.Num());
}

void UAIBatchProcessor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up all active timers
    if (UWorld* World = GetWorld()) {
        FTimerManager& TimerManager = World->GetTimerManager();
        for (auto& TimerPair : BatchProcessingTimers) {
            TimerManager.ClearTimer(TimerPair.Value);
        }
    }

    BatchProcessingTimers.Empty();
    BatchData.Empty();
    AIToBatchMapping.Empty();

    UE_LOG(LogAIBatchProcessor, Log, TEXT("AI Batch Processor cleanup completed"));

    Super::EndPlay(EndPlayReason);
}

void UAIBatchProcessor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update batch processing timers based on current performance load
    UpdateBatchTimers();

    // Perform load balancing if performance monitoring is enabled
    if (bEnablePerformanceMonitoring) {
        static float LoadBalanceTimer = 0.0f;
        LoadBalanceTimer += DeltaTime;

        // Rebalance batches every 2 seconds to maintain optimal performance
        if (LoadBalanceTimer >= 2.0f) {
            RebalanceBatches();
            CleanupEmptyBatches();
            LoadBalanceTimer = 0.0f;
        }
    }
}

void UAIBatchProcessor::InitializeBatchProcessor()
{
    // Find the AI LOD Manager for integration
    if (UWorld* World = GetWorld()) {
        if (AGameStateBase* GameState = World->GetGameState()) {
            LODManager = GameState->FindComponentByClass<UAILODManager>();
            if (!LODManager) {
                UE_LOG(LogAIBatchProcessor, Warning, TEXT("AI LOD Manager not found - batch processing may be suboptimal"));
            }
        }
    }

    // Start batch processing timers for each priority level
    for (auto& BatchPair : BatchData) {
        FString BatchName = BatchPair.Value.BatchName;
        EAIProcessingPriority Priority = BatchPair.Key;

        // Calculate processing interval based on priority
        float ProcessingInterval = BaseBatchProcessingInterval * (static_cast<float>(Priority) + 1.0f);

        if (UWorld* World = GetWorld()) {
            FTimerHandle& TimerHandle = BatchProcessingTimers.FindOrAdd(BatchName);
            World->GetTimerManager().SetTimer(
                TimerHandle,
                [this, BatchName]() {
                    if (BatchData.Contains(GetPriorityFromBatchName(BatchName))) {
                        FAIBatchData& Batch = BatchData[GetPriorityFromBatchName(BatchName)];

                        // Convert TObjectPtr array to regular pointer array for UFUNCTION call
                        TArray<APortalDefenseAIController*> RegularPointers;
                        RegularPointers.Reserve(Batch.AIControllers.Num());
                        for (const TObjectPtr<APortalDefenseAIController>& Controller : Batch.AIControllers) {
                            if (IsValid(Controller)) {
                                RegularPointers.Add(Controller);
                            }
                        }

                        ProcessBatch(RegularPointers, BatchName);
                    }
                },
                ProcessingInterval,
                true);
        }
    }

    UE_LOG(LogAIBatchProcessor, Log, TEXT("Batch processing timers initialized for %d batches"), BatchData.Num());
}

void UAIBatchProcessor::ProcessBatch(const TArray<APortalDefenseAIController*>& AIControllers, const FString& BatchName)
{
    if (AIControllers.Num() == 0) {
        return;
    }

    // Track processing start time for performance monitoring
    double StartTime = FPlatformTime::Seconds();
    int32 ProcessedThisFrame = 0;

    // Process each AI controller in the batch
    for (auto* AIController : AIControllers) {
        if (IsValid(AIController)) {
            ProcessAIController(AIController);
            ProcessedThisFrame++;

            // Check if we've exceeded our frame time budget
            double CurrentTime = FPlatformTime::Seconds();
            double ElapsedTime = (CurrentTime - StartTime) * 1000.0; // Convert to milliseconds

            if (ElapsedTime >= MaxProcessingTimePerFrame) {
                UE_LOG(LogAIBatchProcessor, VeryVerbose, TEXT("Frame time budget exceeded for batch %s, processed %d/%d AI controllers"),
                    *BatchName, ProcessedThisFrame, AIControllers.Num());
                break;
            }
        }
    }

    // Update performance metrics
    double EndTime = FPlatformTime::Seconds();
    float ProcessingTime = static_cast<float>((EndTime - StartTime) * 1000.0);

    if (bEnablePerformanceMonitoring) {
        MonitorBatchPerformance(BatchName, ProcessingTime);
    }

    ProcessedAICount += ProcessedThisFrame;

    UE_LOG(LogAIBatchProcessor, VeryVerbose, TEXT("Processed batch %s: %d AI controllers in %.2fms"),
        *BatchName, ProcessedThisFrame, ProcessingTime);
}

void UAIBatchProcessor::ProcessAIController(APortalDefenseAIController* AIController)
{
    if (!IsValid(AIController)) {
        return;
    }

    // Update AI behavior through ACF combat behavior component
    if (UACFCombatBehaviourComponent* CombatComponent = AIController->FindComponentByClass<UACFCombatBehaviourComponent>()) {
        // Update combat behavior state if available
        if (CombatComponent->HasValidTarget()) {
            // AI has a target - process combat behavior
            CombatComponent->UpdateCombatBehavior();
        }
    }

    // Update LOD-specific behavior if LOD Manager is available
    if (LODManager) {
        int32 CurrentLOD = LODManager->GetAILODLevel(AIController);
        AIController->UpdateAIBehavior(CurrentLOD);
    } else {
        // Fallback to standard behavior update
        AIController->UpdateAIBehavior(1);
    }
}

void UAIBatchProcessor::AssignAIToBatch(APortalDefenseAIController* AIController, int32 LODLevel)
{
    if (!IsValid(AIController)) {
        return;
    }

    // Calculate appropriate batch priority based on LOD level and AI characteristics
    EAIProcessingPriority Priority = CalculateBatchPriority(AIController, LODLevel);

    // Remove from existing batch if already assigned
    if (AIToBatchMapping.Contains(AIController)) {
        RemoveAIFromBatch(AIController);
    }

    // Add to new batch
    if (BatchData.Contains(Priority)) {
        FAIBatchData& TargetBatch = BatchData[Priority];

        // Check if batch has space
        if (TargetBatch.AIControllers.Num() < TargetBatch.MaxBatchSize) {
            TargetBatch.AIControllers.Add(AIController);
            AIToBatchMapping.Add(AIController, TargetBatch.BatchName);

            UE_LOG(LogAIBatchProcessor, VeryVerbose, TEXT("Assigned AI %s to batch %s (Priority: %d)"),
                *AIController->GetName(), *TargetBatch.BatchName, static_cast<int32>(Priority));
        } else {
            UE_LOG(LogAIBatchProcessor, Warning, TEXT("Batch %s is full, could not assign AI %s"),
                *TargetBatch.BatchName, *AIController->GetName());
        }
    }
}

void UAIBatchProcessor::RemoveAIFromBatch(APortalDefenseAIController* AIController)
{
    if (!IsValid(AIController) || !AIToBatchMapping.Contains(AIController)) {
        return;
    }

    FString BatchName = AIToBatchMapping[AIController];

    // Find and remove from the appropriate batch
    for (auto& BatchPair : BatchData) {
        if (BatchPair.Value.BatchName == BatchName) {
            BatchPair.Value.AIControllers.Remove(AIController);
            break;
        }
    }

    AIToBatchMapping.Remove(AIController);

    UE_LOG(LogAIBatchProcessor, VeryVerbose, TEXT("Removed AI %s from batch %s"),
        *AIController->GetName(), *BatchName);
}

FString UAIBatchProcessor::GetAIBatchName(APortalDefenseAIController* AIController)
{
    if (AIToBatchMapping.Contains(AIController)) {
        return AIToBatchMapping[AIController];
    }
    return FString();
}

void UAIBatchProcessor::ForceProcessAllBatches()
{
    UE_LOG(LogAIBatchProcessor, Log, TEXT("Force processing all batches"));

    for (auto& BatchPair : BatchData) {
        // Convert TObjectPtr array to regular pointer array for UFUNCTION call
        TArray<APortalDefenseAIController*> RegularPointers;
        RegularPointers.Reserve(BatchPair.Value.AIControllers.Num());
        for (const TObjectPtr<APortalDefenseAIController>& Controller : BatchPair.Value.AIControllers) {
            if (IsValid(Controller)) {
                RegularPointers.Add(Controller);
            }
        }

        ProcessBatch(RegularPointers, BatchPair.Value.BatchName);
    }
}

EAIProcessingPriority UAIBatchProcessor::CalculateBatchPriority(APortalDefenseAIController* AIController, int32 LODLevel)
{
    // Higher LOD levels (further from player) get lower processing priority
    switch (LODLevel) {
    case 0:
        return EAIProcessingPriority::VeryHigh;
    case 1:
        return EAIProcessingPriority::High;
    case 2:
        return EAIProcessingPriority::Medium;
    case 3:
        return EAIProcessingPriority::Low;
    default:
        return EAIProcessingPriority::VeryLow;
    }
}

int32 UAIBatchProcessor::GetLODLevelFromBatchName(const FString& BatchName)
{
    // Extract LOD level from batch name if it contains priority information
    for (const auto& BatchPair : BatchData) {
        if (BatchPair.Value.BatchName == BatchName) {
            return static_cast<int32>(BatchPair.Key);
        }
    }
    return 1; // Default LOD level
}

void UAIBatchProcessor::UpdateBatchTimers()
{
    // Update processing intervals based on current system load
    for (auto& BatchPair : BatchData) {
        FAIBatchData& Batch = BatchPair.Value;

        // Adjust processing interval based on batch load
        float LoadFactor = static_cast<float>(Batch.AIControllers.Num()) / static_cast<float>(Batch.MaxBatchSize);
        Batch.LoadFactor = LoadFactor;

        // Update timer if load factor has changed significantly
        if (FMath::Abs(Batch.LoadFactor - LoadFactor) > 0.1f) {
            // Restart timer with new interval
            FString BatchName = Batch.BatchName;
            if (UWorld* World = GetWorld()) {
                if (FTimerHandle* TimerHandle = BatchProcessingTimers.Find(BatchName)) {
                    World->GetTimerManager().ClearTimer(*TimerHandle);

                    float NewInterval = BaseBatchProcessingInterval * (1.0f + LoadFactor);
                    World->GetTimerManager().SetTimer(
                        *TimerHandle,
                        [this, BatchName]() {
                            if (BatchData.Contains(GetPriorityFromBatchName(BatchName))) {
                                FAIBatchData& BatchRef = BatchData[GetPriorityFromBatchName(BatchName)];
                                ProcessBatch(BatchRef.AIControllers, BatchName);
                            }
                        },
                        NewInterval,
                        true);
                }
            }
        }
    }
}

void UAIBatchProcessor::MonitorBatchPerformance(const FString& BatchName, float ProcessingTime)
{
    // Update average processing time using exponential moving average
    const float Alpha = 0.1f; // Smoothing factor
    AverageProcessingTime = (Alpha * ProcessingTime) + ((1.0f - Alpha) * AverageProcessingTime);

    // Update batch-specific performance data
    for (auto& BatchPair : BatchData) {
        if (BatchPair.Value.BatchName == BatchName) {
            FAIBatchData& Batch = BatchPair.Value;
            Batch.LastProcessingTime = ProcessingTime;
            Batch.AverageProcessingTime = (Alpha * ProcessingTime) + ((1.0f - Alpha) * Batch.AverageProcessingTime);
            break;
        }
    }

    // Log performance warnings if processing time is excessive
    if (ProcessingTime > MaxProcessingTimePerFrame) {
        UE_LOG(LogAIBatchProcessor, Warning, TEXT("Batch %s exceeded frame time budget: %.2fms > %.2fms"),
            *BatchName, ProcessingTime, MaxProcessingTimePerFrame);
    }
}

void UAIBatchProcessor::RebalanceBatches()
{
    // Find overloaded batches and redistribute AI controllers
    for (auto& BatchPair : BatchData) {
        FAIBatchData& Batch = BatchPair.Value;

        if (Batch.AIControllers.Num() > Batch.MaxBatchSize) {
            // Find less loaded batches of lower priority
            for (auto& TargetBatchPair : BatchData) {
                if (TargetBatchPair.Key > BatchPair.Key && // Lower priority
                    TargetBatchPair.Value.AIControllers.Num() < TargetBatchPair.Value.MaxBatchSize) {
                    // Move AI controllers to less loaded batch
                    int32 ControllersToMove = FMath::Min(
                        Batch.AIControllers.Num() - Batch.MaxBatchSize,
                        TargetBatchPair.Value.MaxBatchSize - TargetBatchPair.Value.AIControllers.Num());

                    for (int32 i = 0; i < ControllersToMove; ++i) {
                        if (Batch.AIControllers.Num() > 0) {
                            TObjectPtr<APortalDefenseAIController> AIController = Batch.AIControllers.Last();
                            Batch.AIControllers.RemoveLast();
                            TargetBatchPair.Value.AIControllers.Add(AIController);

                            // Update mapping
                            AIToBatchMapping[AIController] = TargetBatchPair.Value.BatchName;
                        }
                    }

                    UE_LOG(LogAIBatchProcessor, Log, TEXT("Rebalanced %d AI controllers from %s to %s"),
                        ControllersToMove, *Batch.BatchName, *TargetBatchPair.Value.BatchName);
                }
            }
        }
    }
}

FAIBatchData UAIBatchProcessor::CreateNewBatch(const FString& BatchName, EAIProcessingPriority Priority)
{
    FAIBatchData NewBatch;
    NewBatch.BatchName = BatchName;
    NewBatch.Priority = Priority;
    NewBatch.MaxBatchSize = MaxBatchSize;
    NewBatch.LastProcessingTime = 0.0f;
    NewBatch.AverageProcessingTime = 0.0f;
    NewBatch.LoadFactor = 0.0f;

    return NewBatch;
}

void UAIBatchProcessor::CleanupEmptyBatches()
{
    // Remove AI controllers that are no longer valid
    for (auto& BatchPair : BatchData) {
        FAIBatchData& Batch = BatchPair.Value;
        Batch.AIControllers.RemoveAll([](const TObjectPtr<APortalDefenseAIController>& Controller) {
            return !IsValid(Controller);
        });
    }

    // Clean up invalid mappings
    TArray<TObjectPtr<APortalDefenseAIController>> InvalidControllers;
    for (const auto& MappingPair : AIToBatchMapping) {
        if (!IsValid(MappingPair.Key)) {
            InvalidControllers.Add(MappingPair.Key);
        }
    }

    for (const auto& InvalidController : InvalidControllers) {
        AIToBatchMapping.Remove(InvalidController);
    }
}

EAIProcessingPriority UAIBatchProcessor::GetPriorityFromBatchName(const FString& BatchName)
{
    for (const auto& BatchPair : BatchData) {
        if (BatchPair.Value.BatchName == BatchName) {
            return BatchPair.Key;
        }
    }
    return EAIProcessingPriority::Medium;
}

// Blueprint Accessible Getters
int32 UAIBatchProcessor::GetActiveBatchCount() const
{
    return BatchData.Num();
}

int32 UAIBatchProcessor::GetTotalProcessedAICount() const
{
    return ProcessedAICount;
}

float UAIBatchProcessor::GetAverageProcessingTime() const
{
    return AverageProcessingTime;
}

TArray<FString> UAIBatchProcessor::GetBatchNames() const
{
    TArray<FString> BatchNames;
    for (const auto& BatchPair : BatchData) {
        BatchNames.Add(BatchPair.Value.BatchName);
    }
    return BatchNames;
}

bool UAIBatchProcessor::IsBatchProcessorActive() const
{
    return BatchProcessingTimers.Num() > 0;
}