// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAIController.h"
#include "ACFCoreTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "AIOverseenComponent.generated.h"

// Forward Declarations for ACF Ultimate Integration
class UAIOverlordManager;
class APortalCore;
class UACFThreatManagerComponent;
class UATSTargetingComponent;

/**
 * AI Overseen Component - ACF Ultimate Integration Bridge
 *
 * This component serves as the primary integration layer between Portal game systems
 * and ACF Ultimate's AI framework. It provides seamless coordination between patrol AI,
 * the overlord management system, and ACF's sophisticated combat and behavior systems.
 *
 * Key Features:
 * - Automatic registration with AI Overlord Manager
 * - ACF team management integration
 * - Portal defense coordination
 * - Advanced patrol behavior setup
 * - Player detection and alert systems
 */
UCLASS(ClassGroup = (Portal), meta = (BlueprintSpawnableComponent, DisplayName = "AI Overseen Component"))
class PORTAL_API UAIOverseenComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UAIOverseenComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Core ACF AI Controller Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void InitializeWithController();

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void ReportDeath();

    // ACF Framework Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetCombatTeam(ETeam NewTeam);

    UFUNCTION(BlueprintPure, Category = "AI Overseen")
    bool IsACFCharacter() const;

    // AI Controller Access and Management
    UFUNCTION(BlueprintPure, Category = "AI Overseen")
    AACFAIController* GetACFController() const { return ACFAIController; }

    // Overlord System Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetOverlordTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void ApplyOverlordUpgrade(float MovementMultiplier, float DetectionMultiplier, bool bEnableAdvancedTactics);

    // Patrol System Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetPatrolCenter(FVector Center);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetPatrolRadius(float Radius);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void StartPatrolling();

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void StopPatrolling();

    // Portal Defense Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetPortalDefenseMode(bool bDefendPortal);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void AlertToPlayerPresence(FVector PlayerLocation);

    // Player Detection and Alert System
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void OnPlayerDetected(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void OnPlayerLost();

    // Team and Portal Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetPortalTarget(APortalCore* Portal);

    UFUNCTION(BlueprintPure, Category = "AI Overseen")
    float GetDistanceToPortal() const;

protected:
    // Core ACF Integration Properties
    UPROPERTY(BlueprintReadOnly, Category = "AI Controller")
    TObjectPtr<AACFAIController> ACFAIController;

    // Overlord Registration Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlord Integration", meta = (DisplayName = "Auto Register With Overlord"))
    bool bAutoRegisterWithOverlord = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration", meta = (DisplayName = "Integrate With ACF Teams"))
    bool bIntegrateWithACFTeams = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    ETeam DefaultGuardTeam = ETeam::ETeam2;

    // Patrol Behavior Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol Settings", meta = (DisplayName = "Auto Setup Patrol Behavior"))
    bool bAutoSetPatrolBehavior = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol Settings")
    float DefaultPatrolRadius = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol Settings", meta = (DisplayName = "Use Spawn Location As Patrol Center"))
    bool bUseSpawnLocationAsPatrolCenter = true;

    UPROPERTY(BlueprintReadOnly, Category = "Patrol Settings")
    FVector PatrolCenter = FVector::ZeroVector;

    // Portal Defense Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Defense", meta = (DisplayName = "Defend Portal"))
    bool bDefendPortal = true;

    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    TObjectPtr<APortalCore> AssignedPortal;

    // Player Detection and Alert System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (DisplayName = "Player Detection Range"))
    float PlayerDetectionRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (DisplayName = "Maximum Chase Distance"))
    float MaxChaseDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert System", meta = (DisplayName = "Alert Other Guards"))
    bool bAlertOtherGuards = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert System", meta = (DisplayName = "Alert Radius"))
    float AlertRadius = 1500.0f;

    // State Tracking
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsPlayerDetected = false;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsInCombat = false;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    TObjectPtr<APawn> DetectedPlayer;

    // Performance and Optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (DisplayName = "Enable Advanced AI Features"))
    bool bEnableAdvancedFeatures = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (DisplayName = "Update Frequency"))
    float UpdateFrequency = 0.5f;

private:
    // Internal Setup and Configuration Methods
    void SetupACFIntegration();
    void SetupPatrolBehavior();
    void SetPortalAsDefenseTarget();

    // Event Handlers
    UFUNCTION()
    void OnOwnerDestroyed(AActor* DestroyedActor);

    // ACF Component Access Helpers
    UACFThreatManagerComponent* GetThreatManager() const;
    UATSTargetingComponent* GetTargetingComponent() const;

    // Internal State Management
    void UpdateDetectionState();
    void HandlePlayerProximity();
    void InitializeACFComponents();

    // Timer Handles for Performance Management
    FTimerHandle DetectionUpdateTimer;
    FTimerHandle PatrolUpdateTimer;
};