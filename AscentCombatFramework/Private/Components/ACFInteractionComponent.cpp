// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFInteractionComponent.h"
#include "Interfaces/ACFInteractableInterface.h"
#include <GameFramework/Actor.h>
#include <GameFramework/Pawn.h>
#include "Logging.h"

// Sets default values for this component's properties
UACFInteractionComponent::UACFInteractionComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CollisionChannels.Add(ECollisionChannel::ECC_Pawn);
    SetComponentTickEnabled(true);
    SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UACFInteractionComponent::BeginPlay()
{
    Super::BeginPlay();

    PawnOwner = Cast<APawn>(GetOwner());
    OnComponentBeginOverlap.AddDynamic(this, &UACFInteractionComponent::OnActorEnteredDetector);
    OnComponentEndOverlap.AddDynamic(this, &UACFInteractionComponent::OnActorLeavedDetector);

    if (!PawnOwner) {
        UE_LOG(ACFLog, Error, TEXT("UACFInteractionComponent is Not in a pawn!"));
    }

    if (bAutoEnableOnBeginPlay) {
        EnableDetection(bAutoEnableOnBeginPlay);
    }
}

void UACFInteractionComponent::EnableDetection(bool bIsEnabled)
{
    if (bIsEnabled) {
        InitChannels();
        SetSphereRadius(0.f, false);
        SetSphereRadius(InteractableArea, true);
        SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    } else {
        SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void UACFInteractionComponent::Interact(const FString& interactionType)
{
    ServerInteract(interactionType, currentBestInteractableActor);
    LocalInteract(interactionType);
}

void UACFInteractionComponent::ServerInteract_Implementation(const FString& interactionType /*= ""*/, AActor* bestInteractable)
{
    currentBestInteractableActor = bestInteractable;
    Internal_Interact(interactionType);
}

void UACFInteractionComponent::LocalInteract(const FString& interactionType)
{
    if (currentBestInteractableActor) {
        const bool bImplements = currentBestInteractableActor->GetClass()->ImplementsInterface(UACFInteractableInterface::StaticClass());
        if (bImplements && IACFInteractableInterface::Execute_CanBeInteracted(currentBestInteractableActor, PawnOwner)) {
            IACFInteractableInterface::Execute_OnLocalInteractedByPawn(currentBestInteractableActor, PawnOwner, interactionType);
            OnInteractionSucceded.Broadcast(currentBestInteractableActor);
        }
    }
}

void UACFInteractionComponent::UpdateInteractionArea()
{
    // SetSphereRadius(0.f);
    SetSphereRadius(InteractableArea, true);
}

void UACFInteractionComponent::SetCurrentBestInteractable(class AActor* actor)
{
    if (actor == currentBestInteractableActor) {
        return;
    }

    if (currentBestInteractableActor) {
        OnInteractableUnregistered.Broadcast(currentBestInteractableActor);
        IACFInteractableInterface::Execute_OnInteractableUnregisteredByPawn(currentBestInteractableActor, PawnOwner);
    }
    if (actor) {

        const bool bImplements = actor->GetClass()->ImplementsInterface(UACFInteractableInterface::StaticClass());
        if (bImplements) {

            currentBestInteractableActor = actor;
            IACFInteractableInterface::Execute_OnInteractableRegisteredByPawn(currentBestInteractableActor, PawnOwner);
            OnInteractableRegistered.Broadcast(actor);
        }
    } else {
        currentBestInteractableActor = nullptr;
    }
}

void UACFInteractionComponent::OnActorEnteredDetector(UPrimitiveComponent* _overlappedComponent, AActor* _otherActor, UPrimitiveComponent* _otherComp, int32 _otherBodyIndex, bool _bFromSweep, const FHitResult& _SweepResult)
{
    RegisterInteractable(_otherActor);
}

void UACFInteractionComponent::RegisterInteractable(AActor* otherActor)
{
    const bool bImplements = otherActor->GetClass()->ImplementsInterface(UACFInteractableInterface::StaticClass());
    if (bImplements && PawnOwner && otherActor != PawnOwner) {
        interactables.AddUnique(otherActor);
        RefreshInteractions();
    }
}

void UACFInteractionComponent::UnregisterInteractable(AActor* otherActor)
{
    if (interactables.Contains(otherActor)) {
        interactables.Remove(otherActor);
    }
    RefreshInteractions();
}

void UACFInteractionComponent::OnActorLeavedDetector(UPrimitiveComponent* _overlappedComponent, AActor* otherActor, UPrimitiveComponent* _otherComp, int32 _otherBodyIndex)
{
    UnregisterInteractable(otherActor);
}

void UACFInteractionComponent::RefreshInteractions()
{
    bool bFound = false;
    AActor* newBest = nullptr;
    if (interactables.IsEmpty()) {
        SetCurrentBestInteractable(nullptr);
        return;
    } else {
        // first we see if we can still interact with the current interactable
        if (currentBestInteractableActor && !currentBestInteractableActor->IsPendingKillPending()
            && IACFInteractableInterface::Execute_CanBeInteracted(currentBestInteractableActor, PawnOwner)) {
            newBest = currentBestInteractableActor;
        }
        // otherwise we try with the other interactables in the array
        for (const auto& interactable : interactables) {
            if (!interactable) {

                continue;
            }

            if (newBest && interactable && IACFInteractableInterface::Execute_CanBeInteracted(interactable, PawnOwner)
                && PawnOwner->GetDistanceTo(interactable) < PawnOwner->GetDistanceTo(newBest)) {
                newBest = interactable;
                bFound = true;

            } else if (!newBest && interactable && IACFInteractableInterface::Execute_CanBeInteracted(interactable, PawnOwner)) {
                newBest = interactable;
                bFound = true;
            }
        }
    }

    // if no one is interactable, just set it to nullpts
    if (bFound) {
        SetCurrentBestInteractable(newBest);
    }
}

bool UACFInteractionComponent::HasValidInteractable() const
{
    return currentBestInteractableActor->IsValidLowLevelFast();
}

void UACFInteractionComponent::Internal_Interact(const FString& interactionType)
{
    if (currentBestInteractableActor) {
        const bool bImplements = currentBestInteractableActor->GetClass()->ImplementsInterface(UACFInteractableInterface::StaticClass());
        if (bImplements) {
            IACFInteractableInterface::Execute_OnInteractedByPawn(currentBestInteractableActor, PawnOwner, interactionType);
            OnInteractionSucceded.Broadcast(currentBestInteractableActor);
        }
    }
}

// Called every frame
void UACFInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    RefreshInteractions();
}

void UACFInteractionComponent::AddCollisionChannel(TEnumAsByte<ECollisionChannel> inTraceChannel)
{
    if (!CollisionChannels.Contains(inTraceChannel)) {
        CollisionChannels.Add(inTraceChannel);
        InitChannels();
    }
}

void UACFInteractionComponent::RemoveCollisionChannel(TEnumAsByte<ECollisionChannel> inTraceChannel)
{
    if (CollisionChannels.Contains(inTraceChannel)) {
        CollisionChannels.Remove(inTraceChannel);
        InitChannels();
    }
}

void UACFInteractionComponent::InitChannels()
{
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    for (const auto& channel : CollisionChannels) {
        SetCollisionResponseToChannel(channel, ECR_Overlap);
    }
}
