// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "AIController.h"
#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/ACFTypes.h"
#include "PortalAITypes.h"
#include "TimerManager.h"
#include "UObject/ObjectPtr.h"
#include "AIBatchProcessor.generated.h"

// Forward Declarations
class APortalDefenseAIController;
class UAILODManager;
class UACFCombatBehaviourComponent;

/**
 * AI Batch Data Structure
 * Contains batch processing information for grouped AI controllers
 */
USTRUCT(BlueprintType)
struct PORTAL_API FAIBatchData {
    GENERATED_BODY()

    /** Array of AI Controllers in this batch */
    UPROPERTY(BlueprintReadWrite, Category = "Batch Data")
    TArray<TObjectPtr<APortalDefenseAIController>> AIControllers;

    /** Batch processing priority level */
    UPROPERTY(BlueprintReadWrite, Category = "Batch Data")
    EAIProcessingPriority Priority;

    /** Batch identifier string */
    UPROPERTY(BlueprintReadWrite, Category = "Batch Data")
    FString BatchName;

    /** Last processing timestamp */
    UPROPERTY(BlueprintReadWrite, Category = "Batch Data")
    float LastProcessingTime;

    /** Average processing time for this batch */
    UPROPERTY(BlueprintReadWrite, Category = "Batch Data")
    float AverageProcessingTime;

    /** Maximum AI controllers allowed in this batch */
    UPROPERTY(BlueprintReadWrite, Category = "Batch Data")
    int32 MaxBatchSize;

    /** Current batch processing load factor */
    UPROPERTY(BlueprintReadWrite, Category = "Batch Data")
    float LoadFactor;

    FAIBatchData()
    {
        Priority = EAIProcessingPriority::Medium;
        BatchName = TEXT("DefaultBatch");
        LastProcessingTime = 0.0f;
        AverageProcessingTime = 0.0f;
        MaxBatchSize = 20;
        LoadFactor = 1.0f;
    }
};

/**
 * AI Batch Processor Component
 *
 * Advanced batch processing system for AI controllers to optimize performance
 * in scenarios with large numbers of AI entities. Integrates with ACF Ultimate
 * for seamless combat behavior coordination and LOD management.
 *
 * Features:
 * - Dynamic batch assignment based on LOD levels
 * - Priority-based processing queues
 * - Performance monitoring and load balancing
 * - ACF combat behavior integration
 * - Configurable batch sizes and processing intervals
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class PORTAL_API UAIBatchProcessor : public UActorComponent {
    GENERATED_BODY()

public:
    UAIBatchProcessor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ============================================================================
    // BATCH MANAGEMENT FUNCTIONS
    // ============================================================================

    /** Initialize the batch processing system */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void InitializeBatchProcessor();

    /** Process a specific batch of AI controllers */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void ProcessBatch(const TArray<APortalDefenseAIController*>& AIControllers, const FString& BatchName);

    /** Assign an AI controller to appropriate batch based on LOD level */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void AssignAIToBatch(APortalDefenseAIController* AIController, int32 LODLevel);

    /** Remove AI controller from batch processing */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void RemoveAIFromBatch(APortalDefenseAIController* AIController);

    /** Get the current batch for an AI controller */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    FString GetAIBatchName(APortalDefenseAIController* AIController);

    /** Force immediate processing of all batches */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void ForceProcessAllBatches();

    // ============================================================================
    // BATCH CONFIGURATION
    // ============================================================================

    /** Maximum number of AI controllers per batch */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxBatchSize = 25;

    /** Base processing interval for batches in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float BaseBatchProcessingInterval = 0.1f;

    /** Enable performance monitoring for batch processing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    bool bEnablePerformanceMonitoring = true;

    /** Maximum processing time per frame in milliseconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings", meta = (ClampMin = "1.0", ClampMax = "33.0"))
    float MaxProcessingTimePerFrame = 5.0f;

    /** LOD level multipliers for batch processing intervals */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    TMap<int32, float> LODProcessingMultipliers;

protected:
    // ============================================================================
    // INTERNAL BATCH DATA
    // ============================================================================

    /** Collection of AI batch data organized by priority */
    UPROPERTY()
    TMap<EAIProcessingPriority, FAIBatchData> BatchData;

    /** Mapping of AI controllers to their assigned batches */
    UPROPERTY()
    TMap<TObjectPtr<APortalDefenseAIController>, FString> AIToBatchMapping;

    /** Timer handles for batch processing */
    UPROPERTY()
    TMap<FString, FTimerHandle> BatchProcessingTimers;

    /** Performance tracking data */
    UPROPERTY()
    float AverageProcessingTime = 0.0f;

    /** Total number of processed AI controllers this frame */
    UPROPERTY()
    int32 ProcessedAICount = 0;

    /** Reference to the AI LOD Manager */
    UPROPERTY()
    TObjectPtr<UAILODManager> LODManager;

    // ============================================================================
    // INTERNAL PROCESSING FUNCTIONS
    // ============================================================================

    /** Process individual AI controller within batch context */
    void ProcessAIController(APortalDefenseAIController* AIController);

    /** Update batch processing timers based on current load */
    void UpdateBatchTimers();

    /** Calculate optimal batch assignment for AI controller */
    EAIProcessingPriority CalculateBatchPriority(APortalDefenseAIController* AIController, int32 LODLevel);

    /** Get LOD level from batch name for processing optimization */
    int32 GetLODLevelFromBatchName(const FString& BatchName);

    /** Monitor and log batch processing performance */
    void MonitorBatchPerformance(const FString& BatchName, float ProcessingTime);

    /** Redistribute AI controllers across batches for load balancing */
    void RebalanceBatches();

    /** Create new batch data structure */
    FAIBatchData CreateNewBatch(const FString& BatchName, EAIProcessingPriority Priority);

    /** Clean up empty or invalid batches */
    void CleanupEmptyBatches();

public:
    // ============================================================================
    // BLUEPRINT ACCESSIBLE GETTERS
    // ============================================================================

    /** Get current number of active batches */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    int32 GetActiveBatchCount() const;

    /** Get total number of AI controllers being processed */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    int32 GetTotalProcessedAICount() const;

    /** Get average processing time across all batches */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    float GetAverageProcessingTime() const;

    /** Get batch data for debugging purposes */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    TArray<FString> GetBatchNames() const;

    /** Check if batch processor is currently active */
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    bool IsBatchProcessorActive() const;
};