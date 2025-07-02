// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AIBatchProcessor.h"
#include "Async/Async.h"
#include "Engine/World.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"

TObjectPtr<UAIBatchProcessor> UAIBatchProcessor::InstancePtr = nullptr;

UAIBatchProcessor::UAIBatchProcessor()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f; // Every frame for Maximum LOD processing

    BatchSettings.MaxAIPerBatch = 25;
    BatchSettings.InactiveBatchUpdateRate = 2.0f;
    BatchSettings.MinimalBatchUpdateRate = 1.0f;
    BatchSettings.StandardBatchUpdateRate = 0.5f;
    BatchSettings.HighBatchUpdateRate = 0.1f;
    BatchSettings.MaximumBatchUpdateRate = 0.0f;
    BatchSettings.bUseAsyncProcessing = true;
    BatchSettings.bEnablePerformanceScaling = true;

    AverageProcessingTime = 0.0f;
    CurrentInactiveBatchIndex = 0;
    CurrentMinimalBatchIndex = 0;
    CurrentStandardBatchIndex = 0;
    CurrentHighBatchIndex = 0;

    ProcessingTimes.Reserve(100);
}

void UAIBatchProcessor::BeginPlay()
{
    Super::BeginPlay();

    InstancePtr = this;
    LODManager = UAILODManager::GetInstance(GetWorld());

    InitializeBatchTimers();

    UE_LOG(LogTemp, Log, TEXT("AI Batch Processor initialized - Max batch size: %d"), BatchSettings.MaxAIPerBatch);
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

    // Process Maximum LOD AI every frame
    ProcessMaximumBatch();

    // Track frame time for performance scaling
    LastFrameTime = DeltaTime * 1000.0f; // Convert to milliseconds

    if (BatchSettings.bEnablePerformanceScaling) {
        AdjustBatchSizes(LastFrameTime);
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

    // Process batch portion
    for (int32 i = 0; i < MaxProcessPerFrame; i++) {
        if (CurrentIndex >= Batch.Num()) {
            CurrentIndex = 0; // Wrap around
        }

        AACFAIController* AIController = Batch[CurrentIndex];
        if (AIController && IsValid(AIController)) {
            // Process AI based on LOD level
            switch (LODLevel) {
            case EAILODLevel::Inactive:
                // Minimal processing - just keep alive
                break;

            case EAILODLevel::Minimal:
                // Basic patrol logic
                if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AIController)) {
                    PatrolAI->UpdatePatrolLogic();
                }
                break;

            case EAILODLevel::Standard:
                // Standard AI processing
                if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AIController)) {
                    PatrolAI->UpdatePatrolLogic();
                    PatrolAI->CheckForPlayerThreats();
                }
                break;

            case EAILODLevel::High:
                // Enhanced AI processing
                if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AIController)) {
                    PatrolAI->UpdatePatrolLogic();
                    PatrolAI->CheckForPlayerThreats();
                    PatrolAI->UpdateCombatBehavior();
                }
                break;

            case EAILODLevel::Maximum:
                // Full AI processing - handled in TickComponent
                break;
            }

            ProcessedCount++;
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
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [BatchCopy, LODLevel, this]() {
            for (const TWeakObjectPtr<AACFAIController>& WeakAI : BatchCopy) {
                if (AACFAIController* AIController = WeakAI.Get()) {
                    // Minimal background processing
                    if (LODLevel == EAILODLevel::Minimal) {
                        if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AIController)) {
                            // Basic position updates only
                            AsyncTask(ENamedThreads::GameThread, [PatrolAI]() {
                                if (IsValid(PatrolAI)) {
                                    PatrolAI->UpdatePatrolLogic();
                                }
                            });
                        }
                    }
                }
            }
        });
    } else {
        // Process synchronously for important LOD levels
        ProcessBatch(LODLevel);
    }
}

void UAIBatchProcessor::AdjustBatchSizes(float FrameTime)
{
    if (FrameTime > 20.0f) // Below 50 FPS
    {
        BatchSettings.MaxAIPerBatch = FMath::Max(10, BatchSettings.MaxAIPerBatch - 2);
    } else if (FrameTime < 14.0f) // Above 70 FPS
    {
        BatchSettings.MaxAIPerBatch = FMath::Min(40, BatchSettings.MaxAIPerBatch + 1);
    }
}

void UAIBatchProcessor::OptimizeBatchScheduling()
{
    // Prioritize batches with AI in combat or engaging player
    auto PrioritizeActiveBatch = [](TArray<TObjectPtr<AACFAIController>>& Batch) {
        Batch.Sort([](const TObjectPtr<AACFAIController>& A, const TObjectPtr<AACFAIController>& B) {
            if (!A || !B)
                return false;

            bool AActive = false;
            bool BActive = false;

            if (APortalDefenseAIController* PatrolA = Cast<APortalDefenseAIController>(A)) {
                AActive = PatrolA->IsInCombat() || PatrolA->IsEngagingPlayer();
            }

            if (APortalDefenseAIController* PatrolB = Cast<APortalDefenseAIController>(B)) {
                BActive = PatrolB->IsInCombat() || PatrolB->IsEngagingPlayer();
            }

            return AActive > BActive;
        });
    };

    PrioritizeActiveBatch(CurrentBatches.HighBatch);
    PrioritizeActiveBatch(CurrentBatches.MaximumBatch);
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

    // Set up batch processing timers
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
            if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AIController)) {
                PatrolAI->UpdatePatrolLogic();
                PatrolAI->CheckForPlayerThreats();
                PatrolAI->UpdateCombatBehavior();
                PatrolAI->UpdateTargeting();
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