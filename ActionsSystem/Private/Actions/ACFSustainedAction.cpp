// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Actions/ACFSustainedAction.h"
#include "Actions/ACFBaseAction.h"
#include "Components/ACFActionsManagerComponent.h"
#include "Kismet/GameplayStatics.h"

UACFSustainedAction::UACFSustainedAction()
{
    bBindActionToAnimation = false;
    bAutoCommit = false;
}

void UACFSustainedAction::ReleaseAction()
{
    
    if (ActionState == ESustainedActionState::EStarted) {
        const float secondsHeld = GetActionElapsedTime();
        ActionState = ESustainedActionState::EReleased;
        if (StatisticComp && !bAutoCommit) {
            StatisticComp->ConsumeStatistics(ActionConfig.ActionCost);
        }
        OnActionReleased(secondsHeld);
        PlayActionSection(FinalSectionName);
    }
}

void UACFSustainedAction::PlayActionSection(FName sectionName)
{
    if (ActionsManager) {
        BindAnimationEvents();
        MontageInfo.StartSectionName = sectionName;

        ActionsManager->PlayReplicatedMontage(MontageInfo);
        bIsExecutingAction = true;
    }
}

float UACFSustainedAction::GetActionElapsedTime() const
{
    const float time = UGameplayStatics::GetRealTimeSeconds(CharacterOwner);
    return time - startTime;
}

void UACFSustainedAction::OnActionReleased_Implementation(float elapsedTimeSeconds)
{
}

void UACFSustainedAction::Internal_OnActivated(class UACFActionsManagerComponent* actionmanger, class UAnimMontage* inAnimMontage, const FString& contextString)
{
    startTime = UGameplayStatics::GetRealTimeSeconds(actionmanger);
    ActionState = ESustainedActionState::EStarted;
    Super::Internal_OnActivated(actionmanger, inAnimMontage, contextString);
}

void UACFSustainedAction::Internal_OnDeactivated()
{
    Super::Internal_OnDeactivated();
    ActionState = ESustainedActionState::ENotStarted;
}
