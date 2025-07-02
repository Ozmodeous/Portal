// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "PortalDefenseAIController.h"
#include "AILODManager.generated.h"

UENUM(BlueprintType)
enum class EAILODLevel : uint8 {
    Inactive UMETA(DisplayName = "Inactive"),
    Minimal UMETA(DisplayName = "Minimal"),
    Standard UMETA(DisplayName = "Standard"),
    High UMETA(DisplayName = "High"),
    Maximum UMETA(DisplayName = "Maximum")
};

USTRUCT(BlueprintType)
struct FAILODSettings {
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
    float LODUpdateFrequency = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    bool bUsePlayerPredictiveLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float PredictionRadius = 1500.0f;
};

USTRUCT(BlueprintType)
struct FAILODData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "LOD Data")
    TObjectPtr<AACFAIController> AIController;

    UPROPERTY(BlueprintReadOnly, Category = "LOD Data")
    EAILODLevel CurrentLODLevel = EAILODLevel::Standard;

    UPROPERTY(BlueprintReadOnly, Category = "LOD Data")
    float DistanceToPlayer = 9999.0f;

    UPROPERTY(BlueprintReadOnly, Category = "LOD Data")
    float LODPriority = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "LOD Data")
    bool bInCombat = false;

    UPROPERTY(BlueprintReadOnly, Category = "LOD Data")
    bool bIsEngagingPlayer = false;

    UPROPERTY(BlueprintReadOnly, Category = "LOD Data")
    float LastLODUpdateTime = 0.0f;

    FAILODData()
    {
        AIController = nullptr;
        CurrentLODLevel = EAILODLevel::Standard;
        DistanceToPlayer = 9999.0f;
        LODPriority = 1.0f;
        bInCombat = false;
        bIsEngagingPlayer = false;
        LastLODUpdateTime = 0.0f;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAILODChanged, AACFAIController*, AIController, EAILODLevel, NewLODLevel);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AIFRAMEWORK_API UAILODManager : public UActorComponent {
    GENERATED_BODY()

public:
    UAILODManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Singleton access
    UFUNCTION(BlueprintPure, Category = "AI LOD", meta = (CallInEditor = "true"))
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

    // Events
    UPROPERTY(BlueprintAssignable, Category = "AI LOD")
    FOnAILODChanged OnAILODChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI LOD")
    FAILODSettings LODSettings;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    TArray<FAILODData> RegisteredAI;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    float AverageFrameTime = 16.67f;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    FVector PredictedPlayerPosition = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "AI LOD")
    FVector LastPlayerPosition = FVector::ZeroVector;

private:
    static TObjectPtr<UAILODManager> InstancePtr;

    FTimerHandle LODUpdateTimer;
    FTimerHandle PerformanceMonitorTimer;

    TArray<float> FrameTimes;
    TMap<TObjectPtr<AACFAIController>, float> ForcedLODTimers;

    void StartLODUpdateTimer();
    void StopLODUpdateTimer();
    void OnLODUpdateTimer();
    void MonitorPerformance();
    void UpdatePlayerReference();
    void PredictPlayerPosition();
    EAILODLevel CalculateAILODLevel(const FAILODData& AIData) const;
    void UpdateAILODData(FAILODData& AIData);
    void ProcessForcedLODTimers();
};