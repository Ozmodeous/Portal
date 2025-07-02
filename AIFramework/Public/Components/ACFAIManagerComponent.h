// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACFAIManagerComponent.generated.h"

struct FACFAITicket;

// Delegate called when a new ticket is assigned to an AI Controller.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewTicketAssigned, AController*, AIController);

/**
 * UACFAIManagerComponent
 *
 * This component manages an AI ticketing system that limits the number of AI controllers
 * allowed to attack a single target simultaneously. It ensures better coordination
 * and prevents AI crowding behavior. Needs to be attached to the Game State
 */
UCLASS(ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class AIFRAMEWORK_API UACFAIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	/** Default constructor */
	UACFAIManagerComponent();

	/**
	 * Returns the maximum number of attackers allowed per target.
	 *
	 * @return The current max number of attackers per target.
	 */
	UFUNCTION(BlueprintPure, Category = "ACF|AI")
	int32 GetMaxAttackersPerTarget() const { return MaxAttackersPerTarget; }

	/**
	 * Sets the maximum number of attackers allowed per target.
	 *
	 * @param val - The new maximum number of attackers.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACF|AI")
	void SetMaxAttackersPerTarget(int32 val) { MaxAttackersPerTarget = val; }

	/**
	 * Returns the list of currently active tickets.
	 *
	 * @return An array of active FACFAITicket instances.
	 */
	UFUNCTION(BlueprintPure, Category = "ACF|AI")
	TArray<FACFAITicket> GetActiveTickets() const { return ActiveTickets; }

	/**
	 * Updates the state of all active tickets (e.g., duration countdown).
	 *
	 * @param DeltaTime - The time elapsed since the last update.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACF|AI")
	void UpdateTickets(float DeltaTime);

	/**
	 * Checks whether the given AI controller currently holds an active ticket.
	 *
	 * @param AIController - The controller to check.
	 * @return true if the controller has a ticket, false otherwise.
	 */
	UFUNCTION(BlueprintPure, Category = "ACF|AI")
	bool HasTicket(AController* AIController) const;

	/**
	 * Attempts to assign a ticket to the given AI controller for the specified target.
	 *
	 * @param Target - The actor that the AI wants to attack.
	 * @param AIController - The AI controller requesting the ticket.
	 * @param Duration - How long the ticket should remain valid (in seconds).
	 * @return true if the ticket was successfully assigned, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACF|AI")
	bool RequestTicket(AActor* Target, AController* AIController, float Duration);

	/**
	 * Releases the ticket held by the given AI controller, if any.
	 *
	 * @param AIController - The AI controller whose ticket should be released.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACF|AI")
	void ReleaseTicket(AController* AIController);

	/** Called when a new ticket is successfully assigned to an AI Controller. */
	UPROPERTY(BlueprintAssignable, Category = "ACF|AI")
	FOnNewTicketAssigned OnNewTicketAssigned;

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** The maximum number of attackers allowed to target the same actor simultaneously */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF|AI")
	int32 MaxAttackersPerTarget = 1;

	/** The list of currently active tickets issued to AI controllers */
	UPROPERTY(BlueprintReadOnly, Category = "ACF|AI")
	TArray<FACFAITicket> ActiveTickets;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
