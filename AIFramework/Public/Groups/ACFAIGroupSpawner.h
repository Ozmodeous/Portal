// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

// Include necessary headers for the class functionality, including ACF-specific types.
#include "ACFPatrolPath.h"
#include "CoreMinimal.h"
#include "Game/ACFTypes.h"
#include "GameFramework/Actor.h"
#include <GameFramework/Pawn.h>
#include <GameplayTagContainer.h>
#include <GenericTeamAgentInterface.h>
#include "ACFAITypes.h"
#include "ACFCoreTypes.h"

#include "ACFAIGroupSpawner.generated.h"

// Forward declarations for classes used in the spawner.
class UACFGroupAIComponent;
class USceneComponent;

/**
 *  AI Group Spawner class.
 *
 * This class is responsible for spawning and managing a group of AI agents.
 * It extends APawn and implements both IGenericTeamAgentInterface and IACFEntityInterface,
 * ensuring integration with the team management system and combat entity functionalities
 * defined within ACF.
 */
UCLASS()
class AIFRAMEWORK_API AACFAIGroupSpawner : public APawn, public IGenericTeamAgentInterface, public IACFEntityInterface {
    GENERATED_BODY()

public:
    /**
     *  Default constructor.
     *
     * Initializes default properties and prepares the object for in-game use.
     */
    AACFAIGroupSpawner();

protected:
    /**
     * * Called when the game starts or when the actor is spawned.
     *
     * This overridden method is used to initialize specific properties and, if enabled,
     * automatically spawn the AI group at the beginning of the game.
     */
    virtual void BeginPlay() override;

    /**
     * * Component that handles the AI group's behavior and logic.
     *
     * This component manages the coordination of AI agents that constitute the group,
     * including their collective behaviors and interactions.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACF)
    TObjectPtr<UACFGroupAIComponent> AIGroupComponent;

    /**
     * * Flag indicating whether the AI group should be spawned automatically at BeginPlay.
     *
     * When set to true, the AI group will be spawned immediately upon initialization.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    bool bSpawnOnBeginPlay = false;

    /**
     * * Flag that determines whether the spawner's position should be updated periodically.
     *
     * Enabling this flag causes the spawner's position to be recalculated and updated at fixed intervals.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    bool bUpdateSpawnerPosition = false;

    /**
     * * Time interval (in seconds) for updating the group's position.
     *
     * This value is editable in Blueprint only if bUpdateSpawnerPosition is true.
     */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUpdateSpawnerPosition"), BlueprintReadOnly, Category = ACF)
    float UpdateGroupPositionTimeInterval = 1.f;

    // Interface implementations for team management and combat entity functionalities.

    /**
     * * Sets the generic team ID for this actor.
     *
     * @param InTeamID The team ID to be assigned to this actor.
     */
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;

    /**
     * * Retrieves the generic team ID associated with this actor.
     *
     * @return The current team ID.
     */
    virtual FGenericTeamId GetGenericTeamId() const override;

    /**
     * * Retrieves the combat team for the entity.
     *
     * This function is part of the combat entity interface, returning the team to which the entity belongs.
     *
     * @return The combat team.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    ETeam GetEntityCombatTeam() const;
    virtual ETeam GetEntityCombatTeam_Implementation() const override
    {
        return GetCombatTeam();
    }

    /**
     * * Checks if the entity is alive.
     *
     * Part of the combat entity interface; returns true if the group has one or more agents.
     *
     * @return True if the entity is considered alive; false otherwise.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    bool IsEntityAlive() const;
    virtual bool IsEntityAlive_Implementation() const override
    {
        return GetGroupSize() > 0;
    }

    /**
     * * Assigns a combat team to the entity.
     *
     * This function allows for dynamic assignment of the combat team as part of the combat entity interface.
     *
     * @param inCombatTeam The combat team to assign.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void AssignTeamToEntity(ETeam inCombatTeam);
    virtual void AssignTeamToEntity_Implementation(ETeam inCombatTeam) override
    {
        return SetGenericTeamId(uint8(inCombatTeam));
    }

    /**
     * * Retrieves the extent radius of the entity.
     *
     * This function can be used to determine the spatial bounds of the entity.
     *
     * @return The extent radius, or -1.f if not defined.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    float GetEntityExtentRadius() const;
    virtual float GetEntityExtentRadius_Implementation() const override
    {
        return -1.f;
    }

public:

    /**
     * * Returns the current size of the AI group.
     *
     * The group size typically reflects the number of active agents.
     *
     * @return The number of agents in the group.
     */
    UFUNCTION(BlueprintPure, Category = ACF)
    int32 GetGroupSize() const;

    /**
     * * Provides read-only access to the AI Group component.
     *
     * @return A pointer to the AIGroupComponent.
     */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UACFGroupAIComponent* GetAIGroupComponent() const { return AIGroupComponent; }

    /**
     * * Retrieves the combat team associated with this spawner.
     *
     * @return The combat team.
     */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE ETeam GetCombatTeam() const { return CombatTeam; }

    /**
     * * Retrieves the agent corresponding to the specified index.
     *
     * @param index The index of the agent to retrieve.
     * @param outAgent Output parameter to store the agent's information.
     * @return True if an agent exists at the specified index; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool GetAgentWithIndex(int32 index, FAIAgentsInfo& outAgent) const;

    /**
     * * Returns the agent closest to a given location.
     *
     * This function can be used to determine which agent is nearest to a particular point in the world.
     *
     * @param location The reference location for the proximity search.
     * @return A pointer to the nearest agent, or nullptr if no agents are present.
     */
    UFUNCTION(BlueprintCallable, Category = ACF)
    AACFCharacter* GetAgentNearestTo(const FVector& location) const;

private:
    /**
     * * The combat team assigned to this spawner.
     *
     * The default team is set to ETeam::ETeam1.
     */
    UPROPERTY()
    ETeam CombatTeam = ETeam::ETeam1;

    /**
     * * Initiates periodic updates for the spawner's position.
     *
     * This function starts a timer that triggers position updates at regular intervals.
     */
    UFUNCTION()
    void StartUpdatePos();

    /**
     * * Terminates the periodic position updates.
     *
     * Clears the timer responsible for updating the spawner's position.
     */
    UFUNCTION()
    void StopUpdatePos();

    /**
     * * Adjusts the spawner's position based on group dynamics or external factors.
     *
     * This function recalculates and applies a new position for the spawner.
     */
    UFUNCTION()
    void AdjustSpawnerPos();

    /**
     * * Flag indicating whether position checking is currently active.
     *
     * Used internally to control the update process.
     */
    bool bCheckPos = false;

    /**
     * * Timer handle for managing the periodic update of the spawner's position.
     *
     * This handle is used to start and stop the timer that updates the group's position.
     */
    FTimerHandle timerHandle;
};
