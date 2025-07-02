// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Actions/ACFComboAction.h"

void UACFComboAction::OnActionStarted_Implementation(const FString& contextString /*= ""*/)
{
    bSuccesfulCombo = false;
    Super::OnActionStarted_Implementation();
}

FName UACFComboAction::GetMontageSectionName_Implementation()
{

    if (animMontage) {
        const FName SectionName = animMontage->GetSectionName(CurrentComboIndex);

        if (SectionName != NAME_None) {
            return SectionName;
        } else {
            CurrentComboIndex = 0;
            return animMontage->GetSectionName(CurrentComboIndex);
        }
    }
    return NAME_None;
}

void UACFComboAction::OnActionEnded_Implementation()
{
    Super::OnActionEnded_Implementation();

    if (!bSuccesfulCombo) {
        CurrentComboIndex = 0;
    }
}

void UACFComboAction::OnActionTransition_Implementation(class UACFBaseAction* previousState)
{
    if (previousState && previousState->IsA(GetClass())) {
        if (animMontage && CurrentComboIndex >= animMontage->CompositeSections.Num()) {
            CurrentComboIndex = 0;
        } else {
            CurrentComboIndex++;
        }
        bSuccesfulCombo = true;
    } else {
        CurrentComboIndex = 0;
        bSuccesfulCombo = false;
    }
}
