// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFDefenseStanceComponent.h"
#include "ACFCCTypes.h"
#include "ARSStatisticsComponent.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFBlockComponent.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Components/ACFEquipmentComponent.h"
#include "Game/ACFFunctionLibrary.h"
#include "Items/ACFWeapon.h"
#include "Net/UnrealNetwork.h"
#include "Logging.h"


// Sets default values for this component's properties
UACFDefenseStanceComponent::UACFDefenseStanceComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    // ...
}

void UACFDefenseStanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UACFDefenseStanceComponent, bIsInDefensePosition);
    DOREPLIFETIME(UACFDefenseStanceComponent, bCounterGate);
    DOREPLIFETIME(UACFDefenseStanceComponent, bParryGate);
}

// Called when the game starts
void UACFDefenseStanceComponent::BeginPlay()
{
    Super::BeginPlay();

    statComp = GetOwner()->FindComponentByClass<UARSStatisticsComponent>();
    ensure(statComp);
    locComp = GetOwner()->FindComponentByClass<UACFCharacterMovementComponent>();

    UACFEquipmentComponent* equipComp = GetOwner()->FindComponentByClass<UACFEquipmentComponent>();
    if (equipComp && !equipComp->OnEquipmentChanged.IsAlreadyBound(this, &UACFDefenseStanceComponent::HandleEquipmentChanged)) {
        equipComp->OnEquipmentChanged.AddDynamic(this, &UACFDefenseStanceComponent::HandleEquipmentChanged);
    }
}

void UACFDefenseStanceComponent::HandleEquipmentChanged(const FEquipment& Equipment)
{
    if (bIsInDefensePosition) {
        StopDefending();
    }
}

bool UACFDefenseStanceComponent::CanStartDefensePosition() const
{
    if (!statComp) {
        return false;
    }

    if (statComp->GetCurrentValueForStatitstic(DamagedStatistic) < MinimumDamageStatisticToStartBlocking && !bParryGate) {
        return false;
    }

    return IsValid(TryGetBestBlockComp());
}

void UACFDefenseStanceComponent::StartDefending_Implementation()
{
    if (CanStartDefensePosition()) {
        currentBlockComp = TryGetBestBlockComp();
        if (locComp) {
            locComp->ActivateLocomotionStance(EMovementStance::EBlock);
        }
        bIsInDefensePosition = true;
        statComp->AddAttributeSetModifier(currentBlockComp->GetDefendingModifier());
        OnDefenseStanceChanged.Broadcast(bIsInDefensePosition);
    }
}

void UACFDefenseStanceComponent::StopDefending_Implementation()
{
    if (!statComp) {
        return;
    }
    if (!bIsInDefensePosition) {
        return;
    }

    if (locComp) {
        locComp->DeactivateLocomotionStance(EMovementStance::EBlock);
    }
    bIsInDefensePosition = false;
    statComp->RemoveAttributeSetModifier(currentBlockComp->GetDefendingModifier());
    OnDefenseStanceChanged.Broadcast(bIsInDefensePosition);
}

bool UACFDefenseStanceComponent::TryBlockIncomingDamage(const FACFDamageEvent& damageEvent, float damageToBlock, FGameplayTag& outAction)
{
    if (!CanBlockDamage(damageEvent)) {
        return false;
    }

    const bool bUsingDamagedStat = DamagedStatistic != FGameplayTag();
    if (bUsingDamagedStat && !bParryGate) {
        statComp->ModifyStat(FStatisticValue(DamagedStatistic, -damageToBlock * currentBlockComp->GetDamagedStatisticMultiplier()));
    }
    if (!damageEvent.DamageReceiver) {
        return false;
    }
    UACFActionsManagerComponent* actionsManager = damageEvent.DamageReceiver->FindComponentByClass<UACFActionsManagerComponent>();
    if (statComp->GetCurrentValueForStatitstic(DamagedStatistic) > 0.f || !bUsingDamagedStat || bParryGate) {

        if (!bParryGate) {
            outAction = ActionToBeTriggeredOnBlock;
        }

        if (actionsManager && bCounterGate) {
            actionsManager->TriggerAction(CounterAttackAction, EActionPriority::EHigh);
        }
        OnDamageBlocked.Broadcast(damageEvent);
        return true;
    } else {
        if (actionsManager) {
            actionsManager->TriggerAction(ActionToBeTriggeredOnDefenceBreak, EActionPriority::EHigh);
        }
        StopDefending();
        return false;
    }
}

bool UACFDefenseStanceComponent::CanBlockDamage(const FACFDamageEvent& damageEvent) const
{
    if (!statComp) {
        UE_LOG(ACFLog, Error, TEXT("Missing Stat Component! - UACFDefenseStanceComponent::CanBlockDamage"));
        return false;
    }

    if (!bIsInDefensePosition) {
        return false;
    }

    if (!TryGetBestBlockComp()) {
        return false;
    }

    if ((uint8)damageEvent.DamageDirection != (uint8)currentBlockComp->GetBlockDirection() && currentBlockComp->GetBlockDirection() != EActionDirection::EveryDirection) {
        return false;
    }

    const UACFActionsManagerComponent* ownerActions = GetOwner()->FindComponentByClass<UACFActionsManagerComponent>();
    // you can't block while doing other actions

    if (!ownerActions || !ownerActions->CanExecuteAction(ActionToBeTriggeredOnBlock)) {

        UE_LOG(ACFLog, Warning, TEXT("Impossible to execute block action! - UACFDefenseStanceComponent::CanBlockDamage"));
        return false;
    }

    const bool IsExecutingOtherActions = ownerActions->IsPerformingAction() && (int32)ownerActions->GetCurrentActionPriority() >= (int32)ActionToBeTriggeredOnBlockPriority;

    if (IsExecutingOtherActions && !IsParrying()) {
        return false;
    }

    return currentBlockComp->GetBlockableDamages().Contains(damageEvent.DamageClass);
}

void UACFDefenseStanceComponent::StartParry()
{
    bParryGate = true;
    StartDefending();
}

void UACFDefenseStanceComponent::StopParry()
{
    bParryGate = false;
    StopDefending();
}

bool UACFDefenseStanceComponent::CanParry() const
{
    if (bParryGate) {
        return false;
    }
    const UACFBlockComponent* blockComp = TryGetBestBlockComp();
    if (blockComp) {
        return blockComp->GetCanParry() && CanStartDefensePosition();
    }

    return false;
}

bool UACFDefenseStanceComponent::CanCounterAttack(const FACFDamageEvent& incomingDamage)
{
    if (!bCounterGate) {
        return false;
    }

    if (!currentBlockComp) {
        return false;
    }

    const UACFActionsManagerComponent* actionsManager = GetOwner()->FindComponentByClass<UACFActionsManagerComponent>();
    if (!actionsManager) {
        return false;
    }

    if (!actionsManager->CanExecuteAction(CounterAttackAction)) {
        return false;
    }
    if (!currentBlockComp->GetCounterableDamages().Contains(incomingDamage.DamageClass)) {
        return false;
    }

    if (!incomingDamage.DamageDealer) {
        return false;
    }

    const IACFEntityInterface* acfEntity = Cast<IACFEntityInterface>(incomingDamage.DamageDealer);
    if (acfEntity && !IACFEntityInterface::Execute_IsEntityAlive(incomingDamage.DamageDealer)) {
        return false;
    }

    return true;
}

bool UACFDefenseStanceComponent::TryCounterAttack(const FACFDamageEvent& incomingDamage, FGameplayTag& outCounterAction)
{
    if (!CanCounterAttack(incomingDamage)) {
        return false;
    }
    outCounterAction = CounterAttackAction;
    OnCounterAttackTriggered.Broadcast();

    return true;
}

UACFBlockComponent* UACFDefenseStanceComponent::TryGetBestBlockComp() const
{
    const UACFEquipmentComponent* equipComp = GetOwner()->FindComponentByClass<UACFEquipmentComponent>();

    if (!equipComp) {
        return GetOwner()->FindComponentByClass<UACFBlockComponent>();
    }

    const AACFWeapon* offhand = equipComp->GetCurrentOffhandWeapon();
    if (offhand) {
        auto blockComp = offhand->FindComponentByClass<UACFBlockComponent>();
        if (blockComp) {
            return blockComp;
        }
    }

    const AACFWeapon* currWeap = equipComp->GetCurrentMainWeapon();

    if (currWeap) {
        auto blockComp = currWeap->FindComponentByClass<UACFBlockComponent>();
        if (blockComp) {
            return blockComp;
        }
    }
    return GetOwner()->FindComponentByClass<UACFBlockComponent>();
}

void UACFDefenseStanceComponent::OnRep_DefPos()
{
    OnDefenseStanceChanged.Broadcast(bIsInDefensePosition);
}
