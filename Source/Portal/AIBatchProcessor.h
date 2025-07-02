// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "AILODManager.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "PortalDefenseAIController.h"
#include "AIBatchProcessor.generated.h"

USTRUCT(BlueprintType)
struct FAIBatchData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Batch Processing")
    TArray<TObjectPtr<AACFAIController>> InactiveBatch;

    UPROPERTY(BlueprintReadOnly, Category = "Batch Processing")
    TArray<TObjectPtr<AACFAIController>> MinimalBatch;

    UPROPERTY(BlueprintReadOnly, Category = "Batch Processing")
    TArray<TObjectPtr<AACFAIController>> StandardBatch;

    UPROPERTY(BlueprintReadOnly, Category = "Batch Processing")
    TArray<TObjectPtr<AACFAIController>> HighBatch;

    UPROPERTY(BlueprintReadOnly, Category = "Batch Processing")
    TArray<TObjectPtr<AACFAIController>> MaximumBatch;

    FAIBatchData()
    {
        InactiveBatch.Empty();
        MinimalBatch.Empty();
        StandardBatch.Empty();
        HighBatch.Empty();
        MaximumBatch.Empty();
    }
};

USTRUCT(BlueprintType)
struct FAIBatchSettings {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    int32 MaxAIPerBatch = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    float InactiveBatchUpdateRate = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    float MinimalBatchUpdateRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    float StandardBatchUpdateRate = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    float HighBatchUpdateRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    float MaximumBatchUpdateRate = 0.0f; // Every tick

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    bool bUseAsyncProcessing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch Settings")
    bool bEnablePerformanceScaling = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBatchProcessed, EAILODLevel, LODLevel, int32, ProcessedCount);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AIFRAMEWORK_API UAIBatchProcessor : public UActorComponent {
    GENERATED_BODY()

public:
    UAIBatchProcessor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Singleton access
    UFUNCTION(BlueprintPure, Category = "AI Batch Processing")
    static UAIBatchProcessor* GetInstance(UWorld* World);

    // Batch Management
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void UpdateBatches();

    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void ProcessBatch(EAILODLevel LODLevel);

    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void AddAIToBatch(AACFAIController* AIController, EAILODLevel LODLevel);

    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void RemoveAIFromBatch(AACFAIController* AIController, EAILODLevel LODLevel);

    // Async Processing
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void ProcessBatchAsync(EAILODLevel LODLevel);

    // Performance Management
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void AdjustBatchSizes(float FrameTime);

    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void OptimizeBatchScheduling();

    // Analytics
    UFUNCTION(BlueprintPure, Category = "AI Batch Processing")
    int32 GetTotalBatchedAI() const;

    UFUNCTION(BlueprintPure, Category = "AI Batch Processing")
    int32 GetBatchSize(EAILODLevel LODLevel) const;

    UFUNCTION(BlueprintPure, Category = "AI Batch Processing")
    FAIBatchData GetCurrentBatchData() const { return CurrentBatches; }

    UFUNCTION(BlueprintPure, Category = "AI Batch Processing")
    float GetAverageProcessingTime() const { return AverageProcessingTime; }

    // Settings
    UFUNCTION(BlueprintCallable, Category = "AI Batch Processing")
    void SetBatchSettings(const FAIBatchSettings& NewSettings) { BatchSettings = NewSettings; }

    UFUNCTION(BlueprintPure, Category = "AI Batch Processing")
    FAIBatchSettings GetBatchSettings() const { return BatchSettings; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "AI Batch Processing")
    FOnBatchProcessed OnBatchProcessed;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Batch Processing")
    FAIBatchSettings BatchSettings;

    UPROPERTY(BlueprintReadOnly, Category = "AI Batch Processing")
    FAIBatchData CurrentBatches;

    UPROPERTY(BlueprintReadOnly, Category = "AI Batch Processing")
    float AverageProcessingTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "AI Batch Processing")
    TObjectPtr<UAILODManager> LODManager;

private:
    static TObjectPtr<UAIBatchProcessor> InstancePtr;

    FTimerHandle InactiveBatchTimer;
    FTimerHandle MinimalBatchTimer;
    FTimerHandle StandardBatchTimer;
    FTimerHandle HighBatchTimer;

    int32 CurrentInactiveBatchIndex = 0;
    int32 CurrentMinimalBatchIndex = 0;
    int32 CurrentStandardBatchIndex = 0;
    int32 CurrentHighBatchIndex = 0;

    TArray<float> ProcessingTimes;
    float LastFrameTime = 16.67f;

    void InitializeBatchTimers();
    void ClearBatchTimers();
    void ProcessInactiveBatch();
    void ProcessMinimalBatch();
    void ProcessStandardBatch();
    void ProcessHighBatch();
    void ProcessMaximumBatch();
    void UpdateProcessingMetrics(float ProcessingTime);
    void CleanupInvalidAI();
    TArray<TObjectPtr<AACFAIController>>& GetBatchByLOD(EAILODLevel LODLevel);
    const TArray<TObjectPtr<AACFAIController>>& GetBatchByLOD(EAILODLevel LODLevel) const;
};