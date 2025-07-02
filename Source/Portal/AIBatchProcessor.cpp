// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AIBatchProcessor.h"
#include "Async/AsyncWork.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/PlatformFilemanager.h"
#include "TimerManager.h"

TObjectPtr<UAIBatchProcessor> UAIBatchProcessor::InstancePtr = nullptr;

UAIBatchProcessor::UAIBatchProcessor()
{
    PrimaryComponentTick.bCanEverTick = true;
    bWantsInitializeComponent = true;

    // Default batch settings optimized for performance
    BatchSettings.MaxAIPerBatch = 25;
    BatchSettings.InactiveBatchUpdateRate = 2.0f;
    BatchSettings.MinimalBatchUpdateRate = 1.0f;
    BatchSettings.StandardBatchUpdateRate = 0.5f;
    BatchSettings.HighBatchUpdateRate = 0.1f;
    BatchSettings.MaximumBatchUpdateRate = 0.0f; // Every tick
    BatchSettings.bUseAsyncProcessing = true;
    BatchSettings.bEnablePerformanceScaling = true;

    AverageProcessingTime = 0.0f;
    LastFrameTime = 16.67f;
}

void UAIBatchProcessor::BeginPlay()
{
    Super::BeginPlay();

    InstancePtr = this;

    // Get reference to LOD Manager
    LODManager = UAILODManager::GetInstance(GetWorld());
    if (!LODManager) {
        UE_LOG(LogTemp, Warning, TEXT("AIBatchProcessor: Could not find LOD Manager instance"));
    }

    InitializeBatchTimers();

    UE_LOG(LogTemp, Log, TEXT("AI Batch Processor initialized - Max AI per batch: %d"), BatchSettings.MaxAIPerBatch);
}

void UAIBatchProcessor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearBatchTimers();

    if (InstancePtr == this) {
        InstancePtr = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UAIBatchProcessor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    LastFrameTime = DeltaTime * 1000.0f; // Convert to milliseconds

    // Process Maximum LOD AI every tick
    ProcessMaximumBatch();

    // Update batches periodically
    static float BatchUpdateTimer = 0.0f;
    BatchUpdateTimer += DeltaTime;
    if (BatchUpdateTimer >= 1.0f) {
        UpdateBatches();
        BatchUpdateTimer = 0.0f;
    }
}

UAIBatchProcessor* UAIBatchProcessor::GetInstance(UWorld* World)
{
    if (!InstancePtr && World) {
        // Try to find existing instance
        for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
            if (UAIBatchProcessor* BatchProcessor = ActorItr->FindComponentByClass<UAIBatchProcessor>()) {
                InstancePtr = BatchProcessor;
                break;
            }
        }

        // Create new instance if none found
        if (!InstancePtr) {
            if (AGameModeBase* GameMode = World->GetAuthGameMode()) {
                InstancePtr = NewObject<UAIBatchProcessor>(GameMode);
                GameMode->AddInstanceComponent(InstancePtr);
                InstancePtr->RegisterComponent();
            }
        }
    }

    return InstancePtr;
}

void UAIBatchProcessor::UpdateBatches()
{
    if (!LODManager) {
        return;
    }

    // Clear current batches
    CurrentBatches = FAIBatchData();

    // Reorganize AI into batches based on LOD level
    TArray<FAILODData> AIData = LODManager->GetCurrentLODData();

    for (const FAILODData& Data : AIData) {
        if (Data.AIController && IsValid(Data.AIController)) {
            AddAIToBatch(Data.AIController, Data.CurrentLODLevel);
        }
    }

    CleanupInvalidAI();
    OptimizeBatchScheduling();
}

void UAIBatchProcessor::ProcessBatch(EAILODLevel LODLevel)
{
    const double StartTime = FPlatformTime::Seconds();

    TArray<TObjectPtr<AACFAIController>>& Batch = GetBatchByLOD(LODLevel);

    if (Batch.Num() == 0) {
        return;
    }

    const int32 MaxProcessPerFrame = FMath::Min(BatchSettings.MaxAIPerBatch, Batch.Num());
    int32 ProcessedCount = 0;

    // Get starting index for this LOD level
    int32& CurrentIndex = (LODLevel == EAILODLevel::Inactive) ? CurrentInactiveBatchIndex : (LODLevel == EAILODLevel::Minimal) ? CurrentMinimalBatchIndex
        : (LODLevel == EAILODLevel::Standard)                                                                                  ? CurrentStandardBatchIndex
                                                                                                                               : CurrentHighBatchIndex;

    // Process batch starting from current index
    for (int32 i = 0; i < MaxProcessPerFrame && Batch.Num() > 0; ++i) {
        if (CurrentIndex >= Batch.Num()) {
            CurrentIndex = 0; // Wrap around
        }

        if (Batch.IsValidIndex(CurrentIndex)) {
            TObjectPtr<AACFAIController> AIController = Batch[CurrentIndex];

            if (AIController && IsValid(AIController)) {
                if (APortalDefenseAIController* PortalAI = Cast<APortalDefenseAIController>(AIController)) {
                    // Process different types of updates based on LOD level
                    switch (LODLevel) {
                    case EAILODLevel::Inactive:
                        // Minimal processing for inactive AI
                        break;
                    case EAILODLevel::Minimal:
                        PortalAI->UpdatePatrolLogic();
                        break;
                    case EAILODLevel::Standard:
                        PortalAI->UpdatePatrolLogic();
                        PortalAI->Tick(LastFrameTime / 1000.0f); // Convert back to seconds
                        break;
                    case EAILODLevel::High:
                        PortalAI->UpdatePatrolLogic();
                        PortalAI->Tick(LastFrameTime / 1000.0f);
                        PortalAI->UpdateCombatBehavior();
                        break;
                    case EAILODLevel::Maximum:
                        // Full processing handled in ProcessMaximumBatch
                        break;
                    }
                }
                ProcessedCount++;
            } else {
                // Remove invalid AI from batch
                Batch.RemoveAt(CurrentIndex);
                continue;
            }
        }

        CurrentIndex++;
    }

    const double EndTime = FPlatformTime::Seconds();
    const float ProcessingTime = (EndTime - StartTime) * 1000.0f; // Convert to milliseconds

    UpdateProcessingMetrics(ProcessingTime);
    OnBatchProcessed.Broadcast(LODLevel, ProcessedCount);
}

void UAIBatchProcessor::AddAIToBatch(AACFAIController* AIController, EAILODLevel LODLevel)
{
    if (!AIController || !IsValid(AIController)) {
        return;
    }

    TArray<TObjectPtr<AACFAIController>>& Batch = GetBatchByLOD(LODLevel);

    // Remove from other batches first
    CurrentBatches.InactiveBatch.Remove(AIController);
    CurrentBatches.MinimalBatch.Remove(AIController);
    CurrentBatches.StandardBatch.Remove(AIController);
    CurrentBatches.HighBatch.Remove(AIController);
    CurrentBatches.MaximumBatch.Remove(AIController);

    // Add to appropriate batch
    Batch.AddUnique(AIController);
}

void UAIBatchProcessor::RemoveAIFromBatch(AACFAIController* AIController, EAILODLevel LODLevel)
{
    if (!AIController) {
        return;
    }

    TArray<TObjectPtr<AACFAIController>>& Batch = GetBatchByLOD(LODLevel);
    Batch.Remove(AIController);
}

void UAIBatchProcessor::ProcessBatchAsync(EAILODLevel LODLevel)
{
    if (!BatchSettings.bUseAsyncProcessing) {
        ProcessBatch(LODLevel);
        return;
    }

    TArray<TObjectPtr<AACFAIController>>& Batch = GetBatchByLOD(LODLevel);

    if (Batch.Num() == 0) {
        return;
    }

    // Create a copy for async processing
    TArray<TWeakObjectPtr<AACFAIController>> BatchCopy;
    for (const TObjectPtr<AACFAIController>& AI : Batch) {
        if (AI && IsValid(AI)) {
            BatchCopy.Add(AI);
        }
    }

    // Process asynchronously for non-critical LOD levels
    if (LODLevel == EAILODLevel::Inactive || LODLevel == EAILODLevel::Minimal) {
        // Use task graph for async processing
        FFunctionGraphTask::CreateAndDispatchWhenReady([this, BatchCopy, LODLevel]() {
            const double StartTime = FPlatformTime::Seconds();

            for (const TWeakObjectPtr<AACFAIController>& WeakAI : BatchCopy) {
                if (AACFAIController* AIController = WeakAI.Get()) {
                    if (APortalDefenseAIController* PortalAI = Cast<APortalDefenseAIController>(AIController)) {
                        if (LODLevel == EAILODLevel::Minimal) {
                            PortalAI->UpdatePatrolLogic();
                        }
                    }
                }
            }

            const double EndTime = FPlatformTime::Seconds();
            const float ProcessingTime = (EndTime - StartTime) * 1000.0f;

            // Update metrics on game thread
            FFunctionGraphTask::CreateAndDispatchWhenReady([this, ProcessingTime, LODLevel, BatchCopy]() {
                UpdateProcessingMetrics(ProcessingTime);
                OnBatchProcessed.Broadcast(LODLevel, BatchCopy.Num());
            },
                TStatId(), nullptr, ENamedThreads::GameThread);
        },
            TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask);
    } else {
        // Process synchronously for critical LOD levels
        ProcessBatch(LODLevel);
    }
}

void UAIBatchProcessor::AdjustBatchSizes()
{
    if (!BatchSettings.bEnablePerformanceScaling) {
        return;
    }

    // Adjust batch sizes based on performance
    if (AverageProcessingTime > 5.0f) { // If processing takes more than 5ms
        BatchSettings.MaxAIPerBatch = FMath::Max(BatchSettings.MaxAIPerBatch - 2, 10);
        UE_LOG(LogTemp, Log, TEXT("AIBatchProcessor: Reduced batch size to %d due to performance"), BatchSettings.MaxAIPerBatch);
    } else if (AverageProcessingTime < 2.0f) { // If processing takes less than 2ms
        BatchSettings.MaxAIPerBatch = FMath::Min(BatchSettings.MaxAIPerBatch + 1, 50);
        UE_LOG(LogTemp, VeryVerbose, TEXT("AIBatchProcessor: Increased batch size to %d"), BatchSettings.MaxAIPerBatch);
    }
}

void UAIBatchProcessor::OptimizeBatchScheduling()
{
    // Distribute AI evenly across batches
    TArray<EAILODLevel> LODLevels = {
        EAILODLevel::Inactive,
        EAILODLevel::Minimal,
        EAILODLevel::Standard,
        EAILODLevel::High,
        EAILODLevel::Maximum
    };

    for (EAILODLevel LODLevel : LODLevels) {
        TArray<TObjectPtr<AACFAIController>>& Batch = GetBatchByLOD(LODLevel);

        // Sort AI by priority if LOD Manager is available
        if (LODManager) {
            TArray<FAILODData> LODData = LODManager->GetCurrentLODData();

            Batch.Sort([&LODData](const TObjectPtr<AACFAIController>& A, const TObjectPtr<AACFAIController>& B) {
                float PriorityA = 1.0f;
                float PriorityB = 1.0f;

                for (const FAILODData& Data : LODData) {
                    if (Data.AIController == A) {
                        PriorityA = Data.LODPriority;
                    } else if (Data.AIController == B) {
                        PriorityB = Data.LODPriority;
                    }
                }

                return PriorityA > PriorityB;
            });
        }
    }

    // Adjust processing rates based on current performance
    AdjustBatchSizes();
}

int32 UAIBatchProcessor::GetTotalBatchedAI() const
{
    return CurrentBatches.InactiveBatch.Num() + CurrentBatches.MinimalBatch.Num() + CurrentBatches.StandardBatch.Num() + CurrentBatches.HighBatch.Num() + CurrentBatches.MaximumBatch.Num();
}

int32 UAIBatchProcessor::GetBatchSize(EAILODLevel LODLevel) const
{
    const TArray<TObjectPtr<AACFAIController>>& Batch = GetBatchByLOD(LODLevel);
    return Batch.Num();
}

void UAIBatchProcessor::InitializeBatchTimers()
{
    if (!GetWorld()) {
        return;
    }

    FTimerManager& TimerManager = GetWorld()->GetTimerManager();

    // Set up timers for different batch types
    TimerManager.SetTimer(InactiveBatchTimer, this, &UAIBatchProcessor::ProcessInactiveBatch,
        BatchSettings.InactiveBatchUpdateRate, true);

    TimerManager.SetTimer(MinimalBatchTimer, this, &UAIBatchProcessor::ProcessMinimalBatch,
        BatchSettings.MinimalBatchUpdateRate, true);

    TimerManager.SetTimer(StandardBatchTimer, this, &UAIBatchProcessor::ProcessStandardBatch,
        BatchSettings.StandardBatchUpdateRate, true);

    TimerManager.SetTimer(HighBatchTimer, this, &UAIBatchProcessor::ProcessHighBatch,
        BatchSettings.HighBatchUpdateRate, true);
}

void UAIBatchProcessor::ClearBatchTimers()
{
    if (!GetWorld()) {
        return;
    }

    FTimerManager& TimerManager = GetWorld()->GetTimerManager();

    TimerManager.ClearTimer(InactiveBatchTimer);
    TimerManager.ClearTimer(MinimalBatchTimer);
    TimerManager.ClearTimer(StandardBatchTimer);
    TimerManager.ClearTimer(HighBatchTimer);
}

void UAIBatchProcessor::ProcessInactiveBatch()
{
    ProcessBatchAsync(EAILODLevel::Inactive);
}

void UAIBatchProcessor::ProcessMinimalBatch()
{
    ProcessBatchAsync(EAILODLevel::Minimal);
}

void UAIBatchProcessor::ProcessStandardBatch()
{
    ProcessBatch(EAILODLevel::Standard);
}

void UAIBatchProcessor::ProcessHighBatch()
{
    ProcessBatch(EAILODLevel::High);
}

void UAIBatchProcessor::ProcessMaximumBatch()
{
    // Process all Maximum LOD AI every frame
    const double StartTime = FPlatformTime::Seconds();

    for (TObjectPtr<AACFAIController> AIController : CurrentBatches.MaximumBatch) {
        if (AIController && IsValid(AIController)) {
            if (APortalDefenseAIController* PortalAI = Cast<APortalDefenseAIController>(AIController)) {
                PortalAI->UpdatePatrolLogic();
                PortalAI->UpdateCombatBehavior();
                PortalAI->UpdateTargeting();
                PortalAI->Tick(LastFrameTime / 1000.0f);
            }
        }
    }

    const double EndTime = FPlatformTime::Seconds();
    const float ProcessingTime = (EndTime - StartTime) * 1000.0f;

    UpdateProcessingMetrics(ProcessingTime);
    OnBatchProcessed.Broadcast(EAILODLevel::Maximum, CurrentBatches.MaximumBatch.Num());
}

void UAIBatchProcessor::UpdateProcessingMetrics(float ProcessingTime)
{
    ProcessingTimes.Add(ProcessingTime);

    if (ProcessingTimes.Num() > 100) {
        ProcessingTimes.RemoveAt(0);
    }

    // Calculate average
    float Total = 0.0f;
    for (float Time : ProcessingTimes) {
        Total += Time;
    }

    AverageProcessingTime = ProcessingTimes.Num() > 0 ? Total / ProcessingTimes.Num() : 0.0f;
}

void UAIBatchProcessor::CleanupInvalidAI()
{
    auto CleanupBatch = [](TArray<TObjectPtr<AACFAIController>>& Batch) {
        Batch.RemoveAll([](const TObjectPtr<AACFAIController>& AI) {
            return !AI || !IsValid(AI);
        });
    };

    CleanupBatch(CurrentBatches.InactiveBatch);
    CleanupBatch(CurrentBatches.MinimalBatch);
    CleanupBatch(CurrentBatches.StandardBatch);
    CleanupBatch(CurrentBatches.HighBatch);
    CleanupBatch(CurrentBatches.MaximumBatch);
}

TArray<TObjectPtr<AACFAIController>>& UAIBatchProcessor::GetBatchByLOD(EAILODLevel LODLevel)
{
    switch (LODLevel) {
    case EAILODLevel::Inactive:
        return CurrentBatches.InactiveBatch;
    case EAILODLevel::Minimal:
        return CurrentBatches.MinimalBatch;
    case EAILODLevel::Standard:
        return CurrentBatches.StandardBatch;
    case EAILODLevel::High:
        return CurrentBatches.HighBatch;
    case EAILODLevel::Maximum:
        return CurrentBatches.MaximumBatch;
    default:
        return CurrentBatches.StandardBatch;
    }
}

const TArray<TObjectPtr<AACFAIController>>& UAIBatchProcessor::GetBatchByLOD(EAILODLevel LODLevel) const
{
    switch (LODLevel) {
    case EAILODLevel::Inactive:
        return CurrentBatches.InactiveBatch;
    case EAILODLevel::Minimal:
        return CurrentBatches.MinimalBatch;
    case EAILODLevel::Standard:
        return CurrentBatches.StandardBatch;
    case EAILODLevel::High:
        return CurrentBatches.HighBatch;
    case EAILODLevel::Maximum:
        return CurrentBatches.MaximumBatch;
    default:
        return CurrentBatches.StandardBatch;
    }
}