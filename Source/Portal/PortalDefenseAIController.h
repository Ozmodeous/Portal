// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Game/ACFTypes.h"
#include "PortalDefenseAIController.generated.h"

// Forward Declarations
class APortalCore;
class UAILODManager;
class UEliteAIIntelligenceComponent;
class UACFStealthDetectionComponent;

UENUM(BlueprintType)
enum class EPortalAIState : uint8 {
    Patrolling,
    Investigating,
    ChasingPlayer,
    Guarding,
    Returning
};

USTRUCT(BlueprintType)
struct FPortalAIConfig {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PatrolRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DetectionRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReactionTime = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseACFCombatBehavior = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECombatBehaviorType PreferredCombatType = ECombatBehaviorType::EMelee;

    FPortalAIConfig()
    {
        PatrolRadius = 500.0f;
        DetectionRange = 1200.0f;
        ReactionTime = 0.5f;
        bUseACFCombatBehavior = true;
        PreferredCombatType = ECombatBehaviorType::EMelee;
    }
};

UCLASS()
class PORTAL_API APortalDefenseAIController : public AACFAIController {
    GENERATED_BODY()

public:
    APortalDefenseAIController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Core Portal Defense Functions
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPortalTarget(APortalCore* NewTarget);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StartPatrolling(FVector Center, float Radius = 500.0f);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void InvestigateLocation(FVector Location);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void OnPlayerDetected(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void OnPlayerLost();

    // LOD System Integration
    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdatePatrolLogic();

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateCombatBehavior();

    UFUNCTION(BlueprintCallable, Category = "AI LOD")
    void UpdateTargeting();

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    bool IsInCombat() const;

    UFUNCTION(BlueprintPure, Category = "AI LOD")
    bool IsEngagingPlayer() const;

    // Legacy Support Functions
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPatrolCenter(FVector Center) { StartPatrolling(Center, AIConfig.PatrolRadius); }

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetPatrolRadius(float Radius) { AIConfig.PatrolRadius = Radius; }

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StartPatrolling() { StartPatrolling(PatrolCenter, AIConfig.PatrolRadius); }

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StopPatrolling() { GetWorldTimerManager().ClearTimer(PatrolTimer); }

    // Elite AI Integration (using enum from EliteAIIntelligenceComponent.h)
    UFUNCTION(BlueprintCallable, Category = "Elite AI")
    void SetEliteMode(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Elite AI")
    bool IsEliteModeActive() const;

    // ACF Integration Helpers
    UFUNCTION(BlueprintCallable, Category = "ACF Integration")
    void TriggerACFCombatAction(EAICombatState CombatState);

    UFUNCTION(BlueprintPure, Category = "ACF Integration")
    EAICombatState GetCurrentACFCombatState() const;

    // Overlord Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void ReceiveOverlordCommand(const FString& Command);

    UFUNCTION(BlueprintCallable, Category = "AI Overlord")
    void ReportToOverlord(const FString& ReportType, const FVector& Location);

    // Data Access
    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    FPortalAIConfig GetCurrentAIData() const { return AIConfig; }

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void ApplyAIUpgrade(const FPortalAIConfig& NewConfig) { AIConfig = NewConfig; }

    // Getters
    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    EPortalAIState GetCurrentState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    APawn* GetDetectedPlayer() const { return DetectedPlayer; }

protected:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UACFStealthDetectionComponent> StealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UEliteAIIntelligenceComponent> EliteIntelligence;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FPortalAIConfig AIConfig;

    // Elite AI Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bEnableEliteMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    float EliteActivationDistance = 1000.0f;

    // State
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EPortalAIState CurrentState = EPortalAIState::Patrolling;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bEliteSystemsActive = false;

    // Targets and References
    UPROPERTY(BlueprintReadOnly, Category = "Targets")
    TObjectPtr<APortalCore> PortalTarget = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Targets")
    TObjectPtr<APawn> DetectedPlayer = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Targets")
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    // Patrol Data
    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    FVector PatrolCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    TArray<FVector> PatrolPoints;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol")
    int32 CurrentPatrolIndex = 0;

    // Manager References
    UPROPERTY()
    TObjectPtr<UAILODManager> LODManager = nullptr;

private:
    // Internal state
    FTimerHandle PatrolTimer;
    FTimerHandle InvestigationTimer;
    float LastPlayerDetectionTime = 0.0f;

    // Helper functions
    void InitializeComponents();
    void RegisterWithManagers();
    void GeneratePatrolPoints();
    void MoveToNextPatrolPoint();
    void UpdateEliteSystemsActivation();
    bool ShouldActivateEliteSystems() const;
};