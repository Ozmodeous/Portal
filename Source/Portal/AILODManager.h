// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Game/ACFTypes.h"
#include "GameFramework/Character.h"
#include "PortalDefenseAIController.h"
#include "TimerManager.h"
#include "UObject/ObjectPtr.h"
#include "AILODManager.generated.h"

// Forward Declarations
class APortalDefenseAIController;
class ACharacter;
class UAIBatchProcessor;

/**
 * AI LOD Configuration Structure
 * Defines level-of-detail settings for AI performance optimization
 */
USTRUCT(BlueprintType)
struct PORTAL_API FAILODConfig {
    GENERATED_BODY()

    /** Distance thresholds for each LOD level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances", meta = (ClampMin = "0.0"))
    TArray<float> LODDistances;

    /** Update frequencies for each LOD level (updates per second) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Performance", meta = (ClampMin = "0.1", ClampMax = "60.0"))
    TArray<float> LODUpdateFrequencies;

    /** AI behavior complexity multipliers for each LOD level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Performance", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    TArray<float> LODBehaviorComplexity;

    /** Maximum number of AI entities per LOD level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Limits", meta = (ClampMin = "1"))
    TArray<int32> MaxAIPerLOD;

    /** Enable dynamic LOD adjustment based on frame rate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Performance")
    bool bEnableDynamicLODAdjustment;

    /** Target frame rate for dynamic LOD adjustment */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Performance", meta = (ClampMin = "30.0", ClampMax = "120.0"))
    float TargetFrameRate;

    FAILODConfig()
    {
        // Initialize default LOD distances (in Unreal Units)
        LODDistances = { 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f };

        // Initialize default update frequencies (updates per second)
        LODUpdateFrequencies = { 30.0f, 20.0f, 10.0f, 5.0f, 1.0f };

        // Initialize behavior complexity multipliers
        LODBehaviorComplexity = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f };

        // Initialize maximum AI per LOD level
        MaxAIPerLOD = { 20, 30, 50, 100, 200 };

        bEnableDynamicLODAdjustment = true;
        TargetFrameRate = 60.0f;
    }
};

/**
 * AI Performance Metrics Structure
 * Tracks performance data for LOD optimization
 */
USTRUCT(BlueprintType)
struct PORTAL_API FAIPerformanceMetrics {
    GENERATED_BODY()

    /** Current frame rate */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float CurrentFrameRate;

    /** Average frame time over the last second */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageFrameTime;

    /** Total number of AI entities being managed */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 TotalAICount;

    /** Number of AI entities per LOD level */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    TArray<int32> AICountPerLOD;

    /** Processing time per LOD level in milliseconds */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    TArray<float> ProcessingTimePerLOD;

    /** Memory usage for AI systems in MB */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AIMemoryUsage;

    FAIPerformanceMetrics()
    {
        CurrentFrameRate = 60.0f;
        AverageFrameTime = 16.67f;
        TotalAICount = 0;
        AICountPerLOD = { 0, 0, 0, 0, 0 };
        ProcessingTimePerLOD = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        AIMemoryUsage = 0.0f;
    }
};

/**
 * AI LOD Manager Component
 *
 * Comprehensive Level-of-Detail management system for AI controllers to optimize
 * performance in large-scale scenarios. Dynamically adjusts AI behavior complexity,
 * update frequencies, and processing priority based on distance from players and
 * system performance metrics.
 *
 * Features:
 * - Distance-based LOD assignment
 * - Dynamic performance adjustment
 * - AI registry management
 * - Integration with AI Batch Processor
 * - Frame rate monitoring and optimization
 * - Memory usage tracking
 * - Configurable LOD levels and thresholds
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class PORTAL_API UAILODManager : public UActorComponent {
    GENERATED_BODY()

public:
    UAILODManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ============================================================================
    // LOD MANAGEMENT FUNCTIONS
    // ============================================================================

    /** Initialize the LOD management system */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    void InitializeLODSystem();

    /** Register an AI controller for LOD management */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    void RegisterAIController(APortalDefenseAIController* AIController);

    /** Unregister an AI controller from LOD management */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    void UnregisterAIController(APortalDefenseAIController* AIController);

    /** Get the current LOD level for a specific AI controller */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    int32 GetAILODLevel(APortalDefenseAIController* AIController) const;

    /** Update LOD levels for all registered AI controllers */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    void UpdateLODLevels();

    /** Force refresh of the AI registry */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    void RefreshAIRegistry();

    /** Get all registered AI controllers (Blueprint-safe) */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    TArray<APortalDefenseAIController*> GetRegisteredAIControllers() const;

    /** Set player character reference for distance calculations */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    void SetPlayerCharacter(ACharacter* Player);

    // ============================================================================
    // LOD CONFIGURATION
    // ============================================================================

    /** LOD configuration settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Configuration")
    FAILODConfig LODConfig;

    /** Enable automatic player detection */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Configuration")
    bool bAutoDetectPlayer = true;

    /** Update interval for LOD calculations in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Configuration", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float LODUpdateInterval = 0.5f;

    /** Enable performance monitoring and metrics collection */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Configuration")
    bool bEnablePerformanceMonitoring = true;

    /** Performance adjustment sensitivity (0.0 = no adjustment, 1.0 = maximum adjustment) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Configuration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PerformanceAdjustmentSensitivity = 0.5f;

protected:
    // ============================================================================
    // INTERNAL LOD DATA
    // ============================================================================

    /** Array of registered AI controllers */
    UPROPERTY()
    TArray<TObjectPtr<APortalDefenseAIController>> RegisteredAIControllers;

    /** Mapping of AI controllers to their current LOD levels */
    UPROPERTY()
    TMap<TObjectPtr<APortalDefenseAIController>, int32> AIControllerLODLevels;

    /** Reference to the player character for distance calculations */
    UPROPERTY()
    TObjectPtr<ACharacter> PlayerCharacter;

    /** Timer handle for LOD updates */
    UPROPERTY()
    FTimerHandle LODUpdateTimer;

    /** Current performance metrics */
    UPROPERTY()
    FAIPerformanceMetrics PerformanceMetrics;

    /** Reference to AI Batch Processor for integration */
    UPROPERTY()
    TObjectPtr<UAIBatchProcessor> BatchProcessor;

    /** Singleton instance pointer for global access */
    static TObjectPtr<UAILODManager> InstancePtr;

    // ============================================================================
    // FRAME RATE MONITORING
    // ============================================================================

    /** Frame time samples for averaging */
    TArray<float> FrameTimeSamples;

    /** Maximum number of frame samples to keep */
    int32 MaxFrameSamples = 60;

    /** Current frame sample index */
    int32 FrameSampleIndex = 0;

    // ============================================================================
    // INTERNAL PROCESSING FUNCTIONS
    // ============================================================================

    /** Calculate LOD level based on distance to player */
    int32 CalculateLODLevel(APortalDefenseAIController* AIController) const;

    /** Calculate distance between AI controller and player */
    float CalculateDistanceToPlayer(APortalDefenseAIController* AIController) const;

    /** Apply LOD settings to AI controller */
    void ApplyLODToAI(APortalDefenseAIController* AIController, int32 NewLODLevel);

    /** Update performance metrics */
    void UpdatePerformanceMetrics(float DeltaTime);

    /** Adjust LOD configuration based on performance */
    void AdjustLODForPerformance();

    /** Find and set player character automatically */
    void AutoDetectPlayerCharacter();

    /** Clean up invalid AI controller references */
    void CleanupInvalidAIControllers();

    /** Calculate optimal LOD distribution */
    void OptimizeLODDistribution();

    /** Update frame rate monitoring */
    void UpdateFrameRateMonitoring(float DeltaTime);

public:
    // ============================================================================
    // STATIC ACCESS FUNCTIONS
    // ============================================================================

    /** Get the singleton instance of AI LOD Manager */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    static UAILODManager* GetInstance();

    // ============================================================================
    // BLUEPRINT ACCESSIBLE GETTERS
    // ============================================================================

    /** Get current performance metrics */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    FAIPerformanceMetrics GetPerformanceMetrics() const;

    /** Get number of AI controllers at specific LOD level */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    int32 GetAICountAtLODLevel(int32 LODLevel) const;

    /** Get total number of registered AI controllers */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    int32 GetTotalAICount() const;

    /** Check if LOD system is active and functioning */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    bool IsLODSystemActive() const;

    /** Get average frame rate over the last second */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    float GetAverageFrameRate() const;

    /** Get LOD level name for debugging purposes */
    UFUNCTION(BlueprintCallable, Category = "AI LOD Management")
    FString GetLODLevelName(int32 LODLevel) const;

    // ============================================================================
    // BLUEPRINT EVENTS
    // ============================================================================

    /** Called when an AI controller's LOD level changes */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAILODChanged, APortalDefenseAIController*, AIController, int32, OldLODLevel, int32, NewLODLevel);
    UPROPERTY(BlueprintAssignable, Category = "AI LOD Events")
    FOnAILODChanged OnAILODChanged;

    /** Called when performance adjustment occurs */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerformanceAdjustment, float, FrameRate);
    UPROPERTY(BlueprintAssignable, Category = "AI LOD Events")
    FOnPerformanceAdjustment OnPerformanceAdjustment;
};