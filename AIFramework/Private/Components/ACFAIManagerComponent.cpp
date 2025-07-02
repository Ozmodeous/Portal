// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2025. All Rights Reserved.


#include "Components/ACFAIManagerComponent.h"
#include "ACFAITypes.h"

// Sets default values for this component's properties
UACFAIManagerComponent::UACFAIManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UACFAIManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UACFAIManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTickets(DeltaTime);
}


void UACFAIManagerComponent::UpdateTickets(float DeltaTime)
{

    for (FACFAITicket& Ticket : ActiveTickets) {
        Ticket.TimeRemaining -= DeltaTime;
    }
    ActiveTickets.RemoveAll([](const FACFAITicket& Ticket) { return Ticket.TimeRemaining <= 0.f; });
}

bool UACFAIManagerComponent::HasTicket(AController* AIController) const
{
    return ActiveTickets.Contains(AIController);
}

bool UACFAIManagerComponent::RequestTicket(AActor* Target, AController* AIController, float Duration)
{
    if (!Target || !AIController)
        return false;

    int32 CurrentCount = 0;
    for (const FACFAITicket& Ticket : ActiveTickets) {
        if (Ticket.Target == Target && !(Ticket.TimeRemaining <= 0.f)) {
            ++CurrentCount;
        }
    }

    if (CurrentCount >= MaxAttackersPerTarget) {
        return false;
    }

    ActiveTickets.Add(FACFAITicket(Target, AIController, Duration));
    return true;
}

void UACFAIManagerComponent::ReleaseTicket(AController* AIController)
{
    ActiveTickets.RemoveAll([&](const FACFAITicket& Ticket) { return Ticket.AIController == AIController; });
}
