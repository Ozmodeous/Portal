// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AILODManager.generated.h"

// Forward Declarations
class AACFAIController;
class APawn;

UENUM(BlueprintType)
enum class EAILODLevel : uint8 {
    Inactive, // AI is paused, no behavior tree, no perception
    Minimal, // Basic patrol only, reduced tick rate
    Standard, // Normal AI behavior, standard tick rate
    High, // Enhanced behavior, full perception
    Maximum // All systems active, highest priority
};

USTRUCT(BlueprintType)
struct PORTAL_API FAILODSettings {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float InactiveDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float MinimalDistance = 3500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float StandardDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float HighDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float MaximumDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    int32 MaxHighLODAI = 15;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    int32 MaxMaximumLODAI = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float LODUpdateFrequency = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    bool bUsePerformanceScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    bool bUsePlayerPredictiveLOD = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float PlayerPredictionTime = 2.0f;

    FAILODSettings()
    {
        InactiveDistance = 5000.0f;
        MinimalDistance = 3500.0f;
        StandardDistance = 2000.0f;
        HighDistance = 1000.0f;
        MaximumDistance = 500.0f;
        MaxHighLODAI = 15;
        MaxMaximumLODAI = 8;
        LODUpdateFrequency = 0.25f;
        bUsePerformanceScaling = true;
        bUsePlayerPredictiveLOD = false;
        PlayerPredictionTime = 2.0f;
    }
};

USTRUCT(BlueprintType)
struct PORTAL_API FAILODData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    TObjectPtr<AACFAIController> AIController = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    EAILODLevel CurrentLODLevel = EAILODLevel::Standard;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    float DistanceToPlayer = 9999.0f;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    float LODPriority = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    float LastLODUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    bool bInCombat = false;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    bool bIsEngagingPlayer = false;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    bool bForcedHighLOD = false;

    FAILODData()
    {
        AIController = nullptr;
        CurrentLODLevel = EAILODLevel::Standard;
        DistanceToPlayer = 9999.0f;
        LODPriority = 1.0f;
        LastLODUpdateTime = 0.0f;
        bInCombat = false;
        bIsEngagingPlayer = false;
        bForcedHighLOD = false;
    }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UAILODManager : public UActorComponent {
    GENERATED_BODY()

public:
    UAILODManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Singleton access
    UFUNCTION(BlueprintPure, Category = "AI LOD", CallInEditor = true)
    static UAILODManager* GetInstance(UWorld* World);

    // AI Registration
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void RegisterAI(AACFAIController* AIController);

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UnregisterAI(AACFAIController* AIController);

    // LOD Management
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateAILOD();

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void SetAILODLevel(AACFAIController* AIController, EAILODLevel NewLODLevel);

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void ForceHighLOD(AACFAIController* AIController, float Duration = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void ForceMaximumLOD(AACFAIController* AIController, float Duration = 5.0f);

    // Analytics
    UFUNCTION(BlueprintPure, Category = "AI LOD")
    int32 GetRegisteredAICount() const { return RegisteredAI.Num(); }

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    int32 GetAICountByLOD(EAILODLevel LODLevel) const;

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    TArray<FAILODData> GetCurrentLODData() const { return RegisteredAI; }

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    float GetAverageFrameTime() const { return AverageFrameTime; }

    // Settings
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void SetLODSettings(const FAILODSettings& NewSettings) { LODSettings = NewSettings; }

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    FAILODSettings GetLODSettings() const { return LODSettings; }

protected:
    // Core Data
    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    TArray<FAILODData> RegisteredAI;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI LOD")
    FAILODSettings LODSettings;

    // Performance Tracking
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageFrameTime = 16.67f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    TArray<float> RecentFrameTimes;

    // Player Reference Cache
    UPROPERTY()
    TObjectPtr<APawn> CachedPlayerPawn = nullptr;

    UPROPERTY()
    FVector PredictedPlayerPosition = FVector::ZeroVector;

    // Timers
    FTimerHandle LODUpdateTimer;

private:
    // Internal Methods
    void UpdatePlayerReference();
    void CleanupInvalidAI();
    void CalculateLODPriorities();
    void ApplyLODLimits();
    void UpdatePerformanceMetrics();
    void PredictPlayerMovement();

    EAILODLevel CalculateOptimalLOD(const FAILODData& AIData) const;
    float CalculateAIPriority(const FAILODData& AIData) const;
    void SetAILODInternal(FAILODData& AIData, EAILODLevel NewLODLevel);

    void StartLODUpdateTimer();
    void StopLODUpdateTimer();

    // Singleton instance
    static TObjectPtr<UAILODManager> Instance;
};