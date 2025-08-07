// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "PortalAITypes.generated.h"

/**
 * Portal AI Types
 *
 * Shared type definitions for the Portal Defense AI system to avoid circular dependencies
 * and provide common enumerations and structures used across multiple AI components.
 */

/**
 * AI Threat Level Enumeration
 * Defines the current threat assessment level for tactical coordination
 */
UENUM(BlueprintType)
enum class EThreatLevel : uint8 {
    Low UMETA(DisplayName = "Low Threat"),
    Medium UMETA(DisplayName = "Medium Threat"),
    High UMETA(DisplayName = "High Threat"),
    Critical UMETA(DisplayName = "Critical Threat"),
    Extreme UMETA(DisplayName = "Extreme Threat")
};

/**
 * AI Behavior Mode Enumeration
 * Defines the current behavioral mode for AI coordination
 */
UENUM(BlueprintType)
enum class EAIBehaviorMode : uint8 {
    Patrol UMETA(DisplayName = "Patrol Mode"),
    Alert UMETA(DisplayName = "Alert Mode"),
    Combat UMETA(DisplayName = "Combat Mode"),
    Defensive UMETA(DisplayName = "Defensive Mode")
};

/**
 * Portal AI State Enumeration
 * Defines the behavioral states for Portal Defense AI Controllers
 * Integrates seamlessly with ACF Ultimate's AI state management system
 */
UENUM(BlueprintType)
enum class EPortalAIState : uint8 {
    Patrolling UMETA(DisplayName = "Patrolling"),
    Investigating UMETA(DisplayName = "Investigating"),
    ChasingPlayer UMETA(DisplayName = "Chasing Player"),
    Guarding UMETA(DisplayName = "Guarding Portal"),
    Returning UMETA(DisplayName = "Returning to Patrol")
};

/**
 * AI Coordination Mode Enumeration
 * Defines how AI controllers coordinate with each other
 */
UENUM(BlueprintType)
enum class ECoordinationMode : uint8 {
    Independent UMETA(DisplayName = "Independent Operation"),
    Cooperative UMETA(DisplayName = "Cooperative Coordination"),
    Tactical UMETA(DisplayName = "Tactical Formation"),
    Defensive UMETA(DisplayName = "Defensive Positioning"),
    Aggressive UMETA(DisplayName = "Aggressive Assault")
};

/**
 * AI Processing Priority Levels
 * Defines batch processing priority for performance optimization
 */
UENUM(BlueprintType)
enum class EAIProcessingPriority : uint8 {
    VeryHigh UMETA(DisplayName = "Very High Priority"),
    High UMETA(DisplayName = "High Priority"),
    Medium UMETA(DisplayName = "Medium Priority"),
    Low UMETA(DisplayName = "Low Priority"),
    VeryLow UMETA(DisplayName = "Very Low Priority")
};

/**
 * Portal AI Configuration Structure
 * Comprehensive configuration data for Portal Defense AI behavior
 * Optimized for ACF Ultimate combat system integration
 */
USTRUCT(BlueprintType)
struct PORTAL_API FPortalAIConfig {
    GENERATED_BODY()

    /** Base patrol radius around the assigned portal */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float PatrolRadius = 500.0f;

    /** Maximum distance for player detection and engagement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "200.0", ClampMax = "3000.0"))
    float DetectionRange = 1200.0f;

    /** AI reaction time before initiating response (in seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float ReactionTime = 1.5f;

    /** Maximum distance AI will chase player before returning to patrol */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "300.0", ClampMax = "5000.0"))
    float MaxChaseDistance = 2000.0f;

    /** Investigation duration when player location is uncertain */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (ClampMin = "5.0", ClampMax = "60.0"))
    float InvestigationDuration = 15.0f;

    /** Enable ACF Combat Behavior Component integration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF Integration")
    bool bUseACFCombatBehavior = true;

    /** Base movement speed for AI calculations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "50.0", ClampMax = "1000.0"))
    float BaseMovementSpeed = 300.0f;

    /** Enable elite AI capabilities */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI")
    bool bEnableEliteCapabilities = false;

    /** Distance threshold for elite mode activation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite AI", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float EliteActivationDistance = 800.0f;

    FPortalAIConfig()
    {
        PatrolRadius = 500.0f;
        DetectionRange = 1200.0f;
        ReactionTime = 1.5f;
        MaxChaseDistance = 2000.0f;
        InvestigationDuration = 15.0f;
        bUseACFCombatBehavior = true;
        BaseMovementSpeed = 300.0f;
        bEnableEliteCapabilities = false;
        EliteActivationDistance = 800.0f;
    }
};

/**
 * Portal Defense AI Data Structure
 * Runtime data tracking for AI performance and state management
 * Integrated with AI LOD Manager for performance optimization
 */
USTRUCT(BlueprintType)
struct PORTAL_API FPortalAIData {
    GENERATED_BODY()

    /** Current AI configuration settings */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    FPortalAIConfig Config;

    /** Current movement speed (can be modified by upgrades) */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float MovementSpeed = 1.0f;

    /** Enhanced player detection range (modified by intelligence upgrades) */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float PlayerDetectionRange = 1000.0f;

    /** Enable advanced tactical behaviors */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    bool bUseAdvancedPathfinding = false;

    /** Combat accuracy modifier for ACF targeting systems */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float CombatAccuracy = 1.0f;

    /** Response time optimization factor */
    UPROPERTY(BlueprintReadWrite, Category = "AI Data")
    float ResponseTimeMultiplier = 1.0f;

    FPortalAIData()
    {
        Config = FPortalAIConfig();
        MovementSpeed = 1.0f;
        PlayerDetectionRange = 1000.0f;
        bUseAdvancedPathfinding = false;
        CombatAccuracy = 1.0f;
        ResponseTimeMultiplier = 1.0f;
    }
};

/**
 * Tactical Formation Structure
 * Defines positioning and coordination data for AI formations
 */
USTRUCT(BlueprintType)
struct PORTAL_API FTacticalFormation {
    GENERATED_BODY()

    /** Formation identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    FString FormationName;

    /** Relative positions for AI units in the formation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    TArray<FVector> RelativePositions;

    /** Optimal number of AI units for this formation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (ClampMin = "1", ClampMax = "20"))
    int32 OptimalUnitCount;

    /** Formation center position */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    FVector CenterPosition;

    /** Formation facing direction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    FVector FacingDirection;

    /** Maximum formation radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (ClampMin = "50.0"))
    float MaxFormationRadius;

    FTacticalFormation()
    {
        FormationName = TEXT("Default Formation");
        OptimalUnitCount = 4;
        CenterPosition = FVector::ZeroVector;
        FacingDirection = FVector::ForwardVector;
        MaxFormationRadius = 300.0f;

        // Initialize default diamond formation
        RelativePositions = {
            FVector(0.0f, 0.0f, 0.0f), // Center
            FVector(100.0f, 0.0f, 0.0f), // Front
            FVector(-100.0f, 0.0f, 0.0f), // Back
            FVector(0.0f, 100.0f, 0.0f), // Right
            FVector(0.0f, -100.0f, 0.0f) // Left
        };
    }
};
#pragma once
