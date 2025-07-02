#pragma once

#include "ACFAIController.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "AIOverlordManager.generated.h"

USTRUCT(BlueprintType)
struct FPatrolAnalysisData {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ActivePatrolGuards = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AveragePlayerDetectionTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PlayerIncursions = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> PlayerPositions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> GuardDeathLocations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CaptureProgress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SessionDuration = 0.0f;
};

USTRUCT(BlueprintType)
struct FTacticalInsight {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString InsightType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Priority = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> RecommendedRoute;
};

USTRUCT(BlueprintType)
struct FACFAIUpgradeData {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PatrolRadiusMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DetectionRangeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AccuracyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AggressionLevel = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableAdvancedTactics = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanCoordinate = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ResponseTime = 1.0f;
};

UCLASS(BlueprintType)
class PORTAL_API UAIOverlordManager : public UWorldSubsystem {
    GENERATED_BODY()

public:
    UAIOverlordManager();

    // UWorldSubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Subsystem Access
    UFUNCTION(BlueprintCallable, Category = "AI Overlord", CallInEditor)
    static UAIOverlordManager* GetInstance(const UObject* WorldContext);

    // AI Registration
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void RegisterAI(AACFAIController* AIController);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void UnregisterAI(AACFAIController* AIController);

    // Patrol Analysis
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

    // AI Enhancement
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void UpgradePatrolAI();

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void AssignPatrolCoordination();

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void OptimizePatrolRoutes();

    // Intelligence System
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    TArray<FTacticalInsight> GenerateTacticalInsights();

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void UpdateAIIntelligence(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void AdaptToPlayerBehavior();

    // Commands
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

    // Analytics
    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    float GetCurrentIntelligenceLevel() const { return AIIntelligenceLevel; }

    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    int32 GetRegisteredAICount() const { return RegisteredAI.Num(); }

    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    FPatrolAnalysisData GetCurrentAnalysisData() const { return CurrentAnalysisData; }

    UFUNCTION(BlueprintPure, Category = "AI Overlord")
    TArray<FPatrolAnalysisData> GetAnalysisHistory() const { return AnalysisHistory; }

protected:
    // Registered AI Controllers
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TArray<TObjectPtr<AACFAIController>> RegisteredAI;

    // Analysis Data
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TArray<FPatrolAnalysisData> AnalysisHistory;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    FPatrolAnalysisData CurrentAnalysisData;

    // Intelligence Level
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    float AIIntelligenceLevel = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    float IntelligenceGrowthRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    float MaxIntelligenceLevel = 5.0f;

    // Portal Reference
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TObjectPtr<class APortalCore> PortalTarget;

    // Player Tracking
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TArray<FVector> RecentPlayerPositions;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TArray<FVector> PlayerIncursionPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    int32 MaxPlayerPositionHistory = 100;

    // Analysis Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    float AnalysisInterval = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    float PlayerTrackingInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    bool bEnableContinuousAnalysis = true;

    // Session Tracking
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    float SessionStartTime;

    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    int32 TotalPlayerIncursions;

    // ACF Integration Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    FGameplayTag PatrolCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    FGameplayTag AlertCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    FGameplayTag CoordinateCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    FGameplayTag DefaultAIState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overlord")
    FGameplayTag PatrolAIState;

private:
    // Timers
    FTimerHandle AnalysisTimer;
    FTimerHandle PlayerTrackingTimer;

    // Internal Functions
    void StartContinuousAnalysis();
    void PerformRealTimeAnalysis();
    void TrackPlayerMovement();
    FACFAIUpgradeData CalculateAIUpgrades(float IntelligenceLevel);
    void FindPortalTarget();
    void CleanupInvalidAI();
    void AnalyzePlayerBehaviorPatterns();

    // ACF Integration
    void SetACFPatrolBehavior(AACFAIController* AIController, const FACFAIUpgradeData& UpgradeData);
    void SendACFCommand(AACFAIController* AIController, const FGameplayTag& CommandTag);

    UFUNCTION()
    void OnAnalysisTimer();

    UFUNCTION()
    void OnPlayerTrackingTimer();
};