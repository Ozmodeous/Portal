// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Game/ACFTypes.h"
#include "PortalAITypes.h"
#include "TimerManager.h"
#include "UObject/ObjectPtr.h"
#include "AIOverseenComponent.generated.h"

// Forward Declarations
class APortalDefenseAIController;
class APortalCore;

/**
 * AI Overseen Component
 *
 * Advanced tactical coordination system for AI controllers that provides
 * sophisticated group behavior management, threat assessment, and formation
 * control. Integrates seamlessly with ACF Ultimate's AI systems.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class PORTAL_API UAIOverseenComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UAIOverseenComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ============================================================================
    // TACTICAL COORDINATION FUNCTIONS
    // ============================================================================

    /** Initialize the tactical coordination system */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void InitializeTacticalSystem();

    /** Register an AI controller for tactical coordination */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void RegisterManagedAI(APortalDefenseAIController* AIController);

    /** Unregister an AI controller from coordination */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void UnregisterManagedAI(APortalDefenseAIController* AIController);

    /** Update tactical coordination for all managed AI */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void UpdateTacticalCoordination();

    /** Set the current threat level for tactical response */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    void SetThreatLevel(EThreatLevel NewThreatLevel);

    /** Find the Portal Core for defense coordination */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    APortalCore* FindPortalCore();

    // ============================================================================
    // COORDINATION CONFIGURATION
    // ============================================================================

    /** Tactical update rate in updates per second */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordination Settings", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float TacticalUpdateRate = 2.0f;

    /** Maximum number of AI controllers this component can manage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordination Settings", meta = (ClampMin = "1", ClampMax = "50"))
    int32 MaxManagedAI = 10;

    /** Enable elite AI coordination for advanced tactics */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordination Settings")
    bool bEnableEliteCoordination = true;

    /** Enable portal defense behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordination Settings")
    bool bDefendPortal = true;

    /** Threat escalation threshold before changing tactics */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordination Settings", meta = (ClampMin = "1", ClampMax = "10"))
    int32 ThreatEscalationThreshold = 3;

    /** Coordination radius for AI influence */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordination Settings", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
    float CoordinationRadius = 1500.0f;

    /** Maximum defensive radius around portal */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordination Settings", meta = (ClampMin = "100.0", ClampMax = "3000.0"))
    float MaxDefensiveRadius = 800.0f;

protected:
    // ============================================================================
    // INTERNAL COORDINATION DATA
    // ============================================================================

    /** Array of managed AI controllers */
    UPROPERTY()
    TArray<TObjectPtr<APortalDefenseAIController>> ManagedAIControllers;

    /** Reference to the Portal Core for defense coordination */
    UPROPERTY()
    TObjectPtr<APortalCore> PortalCore;

    /** Current threat level assessment */
    UPROPERTY()
    EThreatLevel CurrentThreatLevel = EThreatLevel::Low;

    /** Timer handle for tactical updates */
    UPROPERTY()
    FTimerHandle TacticalUpdateTimer;

    /** Defensive position around portal */
    UPROPERTY()
    FVector DefensivePosition = FVector::ZeroVector;

    // ============================================================================
    // INTERNAL PROCESSING FUNCTIONS
    // ============================================================================

    /** Check if AI controller is eligible for coordination */
    bool IsAIControllerEligible(APortalDefenseAIController* AIController) const;

    /** Configure AI for tactical coordination */
    void ConfigureAIForTacticalCoordination(APortalDefenseAIController* AIController);

    /** Update threat assessment based on current situation */
    void UpdateThreatAssessment();

    /** Calculate optimal defensive positions */
    void CalculateDefensivePositions();

    /** Apply coordination commands to managed AI */
    void ApplyCoordinationCommands();

public:
    // ============================================================================
    // BLUEPRINT ACCESSIBLE GETTERS
    // ============================================================================

    /** Get current number of managed AI controllers */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    int32 GetManagedAICount() const;

    /** Get current threat level */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    EThreatLevel GetCurrentThreatLevel() const;

    /** Check if coordination system is active */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    bool IsCoordinationActive() const;

    /** Get managed AI controllers array */
    UFUNCTION(BlueprintCallable, Category = "AI Coordination")
    TArray<APortalDefenseAIController*> GetManagedAIControllers() const;
};