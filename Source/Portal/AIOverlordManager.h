// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "AIOverlordManager.generated.h"

// Forward Declarations for ACF Ultimate Integration
class APortalCore;
class APortalDefenseAIController;

/**
 * Structured data for comprehensive patrol performance analysis
 * Integrates with ACF Ultimate's AI behavior tracking systems
 */
USTRUCT(BlueprintType)
struct PORTAL_API FPatrolAnalysisData {
    GENERATED_BODY()

    /** Number of actively patrolling ACF AI controllers */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analysis")
    int32 ActivePatrolGuards = 0;

    /** Average time for ACF AI to detect player presence */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analysis")
    float AveragePlayerDetectionTime = 0.0f;

    /** Total recorded player incursion events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analysis")
    int32 PlayerIncursions = 0;

    /** Historical player position tracking for pattern analysis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analysis")
    TArray<FVector> PlayerPositions;

    /** Locations where ACF AI controllers were eliminated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analysis")
    TArray<FVector> GuardDeathLocations;

    /** Current portal capture progress percentage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analysis")
    float CaptureProgress = 0.0f;

    /** Duration of current analysis session */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analysis")
    float SessionDuration = 0.0f;

    FPatrolAnalysisData()
    {
        ActivePatrolGuards = 0;
        AveragePlayerDetectionTime = 0.0f;
        PlayerIncursions = 0;
        CaptureProgress = 0.0f;
        SessionDuration = 0.0f;
    }
};

/**
 * Tactical insight data structure for advanced ACF AI decision making
 * Provides contextual information for enhanced combat behavior
 */
USTRUCT(BlueprintType)
struct PORTAL_API FTacticalInsight {
    GENERATED_BODY()

    /** Classification of tactical insight for ACF behavior trees */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    FString InsightType;

    /** Primary target location for ACF AI coordination */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    FVector TargetLocation;

    /** Priority weighting for ACF action selection */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    float Priority = 1.0f;

    /** Recommended patrol route for ACF navigation system */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    TArray<FVector> RecommendedRoute;

    FTacticalInsight()
    {
        InsightType = TEXT("Standard");
        TargetLocation = FVector::ZeroVector;
        Priority = 1.0f;
    }
};

/**
 * Comprehensive upgrade data structure for ACF AI enhancement
 * Integrates with ACF Ultimate's character progression systems
 */
USTRUCT(BlueprintType)
struct PORTAL_API FACFAIUpgradeData {
    GENERATED_BODY()

    /** Movement speed enhancement multiplier for ACF character controllers */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    float MovementSpeedMultiplier = 1.0f;

    /** Patrol radius expansion factor for ACF AI territories */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    float PatrolRadiusMultiplier = 1.0f;

    /** Detection range enhancement for ACF perception systems */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    float DetectionRangeMultiplier = 1.0f;

    /** Combat accuracy improvement for ACF targeting systems */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    float AccuracyMultiplier = 1.0f;

    /** Aggression level modifier for ACF behavior trees */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    float AggressionLevel = 1.0f;

    /** Enable advanced tactical behaviors in ACF AI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    bool bEnableAdvancedTactics = false;

    /** Allow coordination between ACF AI controllers */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    bool bCanCoordinate = false;

    /** Response time optimization for ACF reactive systems */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
    float ResponseTime = 1.0f;

    FACFAIUpgradeData()
    {
        MovementSpeedMultiplier = 1.0f;
        PatrolRadiusMultiplier = 1.0f;
        DetectionRangeMultiplier = 1.0f;
        AccuracyMultiplier = 1.0f;
        AggressionLevel = 1.0f;
        bEnableAdvancedTactics = false;
        bCanCoordinate = false;
        ResponseTime = 1.0f;
    }
};

/**
 * AI Overlord Manager - Central coordination system for ACF Ultimate AI
 *
 * Manages advanced AI behavior coordination, tactical analysis, and adaptive intelligence
 * for portal defense scenarios. Integrates deeply with ACF Ultimate's AI framework
 * to provide sophisticated enemy coordination and dynamic difficulty scaling.
 */
UCLASS(BlueprintType, meta = (DisplayName = "AI Overlord Manager"))
class PORTAL_API UAIOverlordManager : public UWorldSubsystem {
    GENERATED_BODY()

public:
    UAIOverlordManager();

    // UWorldSubsystem interface - UE 5.5 compliant implementation
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Subsystem Access - FIXED: Removed invalid CallInEditor syntax
    UFUNCTION(BlueprintCallable, Category = "AI Overlord", CallInEditor)
    static UAIOverlordManager* GetInstance(const UObject* WorldContext);

    // ACF AI Registration and Management
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void RegisterAI(AACFAIController* AIController);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void UnregisterAI(AACFAIController* AIController);

    // Patrol Performance Analysis System
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void AnalyzePatrolPerformance();

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void RecordAIDeath(AACFAIController* DeadAI, FVector DeathLocation);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void RecordPlayerPosition(FVector PlayerLocation);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void RecordPlayerIncursion(FVector IncursionLocation);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void UpdateCaptureProgress(float Progress);

    // ACF AI Enhancement and Upgrading
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void UpgradePatrolAI();

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void AssignPatrolCoordination();

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void OptimizePatrolRoutes();

    // Advanced Intelligence and Tactical Systems
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    TArray<FTacticalInsight> GenerateTacticalInsights();

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void UpdateAIIntelligence(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void AdaptToPlayerBehavior();

    // ACF Command System Integration - FIXED: Corrected CallInEditor usage
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void IssueGlobalCommand(const FString& Command, const TArray<FVector>& Parameters);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord", CallInEditor, DisplayName = "Issue Global Command (Simple)")
    void IssueGlobalCommandSimple(const FString& Command) { IssueGlobalCommand(Command, TArray<FVector>()); }

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void IssueSelectiveCommand(const FString& Command, int32 MaxUnits, const TArray<FVector>& Parameters);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord", CallInEditor, DisplayName = "Issue Selective Command (Simple)")
    void IssueSelectiveCommandSimple(const FString& Command, int32 MaxUnits) { IssueSelectiveCommand(Command, MaxUnits, TArray<FVector>()); }

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void AlertNearbyGuards(FVector AlertLocation, float AlertRadius = 1500.0f);

    // Analytics and Data Access
    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    float GetCurrentIntelligenceLevel() const { return AIIntelligenceLevel; }

    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    int32 GetRegisteredAICount() const { return RegisteredAI.Num(); }

    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    FPatrolAnalysisData GetCurrentAnalysisData() const { return CurrentAnalysisData; }

    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    TArray<FPatrolAnalysisData> GetAnalysisHistory() const { return AnalysisHistory; }

protected:
    // ACF AI Controller Registry
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TArray<TObjectPtr<AACFAIController>> RegisteredAI;

    // Performance Analysis Data
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TArray<FPatrolAnalysisData> AnalysisHistory;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    FPatrolAnalysisData CurrentAnalysisData;

    // Dynamic Intelligence System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Intelligence")
    float AIIntelligenceLevel = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Intelligence")
    float IntelligenceGrowthRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Intelligence")
    float MaxIntelligenceLevel = 5.0f;

    // Portal Defense Integration
    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    TObjectPtr<APortalCore> PortalTarget;

    // Player Behavior Tracking
    UPROPERTY(BlueprintReadOnly, Category = "Player Tracking")
    TArray<FVector> RecentPlayerPositions;

    UPROPERTY(BlueprintReadOnly, Category = "Player Tracking")
    TArray<FVector> PlayerIncursionPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Tracking")
    int32 MaxPlayerPositionHistory = 100;

    UPROPERTY(BlueprintReadOnly, Category = "Player Tracking")
    int32 TotalPlayerIncursions = 0;

    // ACF Integration Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    FGameplayTag PatrolCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    FGameplayTag AlertCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    FGameplayTag CoordinateCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    FGameplayTag DefaultAIState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    FGameplayTag PatrolAIState;

    // System Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float AnalysisInterval = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float PlayerTrackingInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableContinuousAnalysis = true;

    // Timer Management
    FTimerHandle AnalysisTimer;
    FTimerHandle PlayerTrackingTimer;
    FTimerHandle IntelligenceUpdateTimer;

    // Session Tracking
    float SessionStartTime = 0.0f;

private:
    // Internal Management Functions
    void FindPortalTarget();
    void StartContinuousAnalysis();
    void CleanupInvalidAI();

    // ACF Integration Helpers
    FACFAIUpgradeData CalculateAIUpgrades(float IntelligenceLevel) const;
    void SetACFPatrolBehavior(AACFAIController* AIController, const FACFAIUpgradeData& UpgradeData);
    void ApplyACFCoordinationBehavior(AACFAIController* AIController, bool bEnableCoordination);

    // Tactical Analysis
    void AnalyzePlayerPatterns();
    void GenerateCounterTactics();
    void UpdateThreatAssessment();
};