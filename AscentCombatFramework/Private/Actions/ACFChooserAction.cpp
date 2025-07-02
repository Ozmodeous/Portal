// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.


#include "Actions/ACFChooserAction.h"

#include "Chooser.h"
#include "ChooserFunctionLibrary.h"
#include "Animation/AnimMontage.h"
#include "Components/ACFActionsManagerComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Logging.h"


UACFChooserAction::UACFChooserAction()
{
    // Constructor logic if needed
}

UAnimMontage* UACFChooserAction::EvaluateChooserTable()
{
    // Ensure the ChooserTable is valid
    if (!ChooserTable)
    {
        UE_LOG(ACFLog, Error, TEXT("Chooser Table is not set for this action!"));
        return nullptr;
    }

    // Create an evaluation context
    FChooserEvaluationContext Context;

    // Evaluate the Chooser Table
    UObject* Result = nullptr;
    UChooserTable::EvaluateChooser(Context, ChooserTable, FObjectChooserBase::FObjectChooserIteratorCallback::CreateLambda([&Result](UObject* InResult)
    {
        Result = InResult;
        return FObjectChooserBase::EIteratorStatus::Stop;
    }));

    if (!Result)
    {
        UE_LOG(ACFLog, Warning, TEXT("Chooser evaluation failed or returned null."));
        return nullptr;
    }

    UAnimMontage* Montage = Cast<UAnimMontage>(Result);
    if (!Montage)
    {
        UE_LOG(ACFLog, Error, TEXT("Chooser result is not a valid AnimMontage!"));
        return nullptr;
    }

    UE_LOG(ACFLog, Log, TEXT("Chooser Table selected montage: %s"), *Montage->GetName());
    return Montage;
}

void UACFChooserAction::SetChooserParams_Implementation()
{
    // Default implementation. To be overridden in Blueprints.
    UE_LOG(LogTemp, Warning, TEXT("SetChooserParams was called! Override this in Blueprints to set parameters."));
}

void UACFChooserAction::Internal_OnActivated(class UACFActionsManagerComponent* ActionManager, UAnimMontage* InAnimMontage, const FString& ContextString)
{
    if (ActionManager) {
        CharacterOwner = Cast<ACharacter>(ActionManager->GetOwner());
    }

    // Allow users to set up chooser parameters dynamically
    SetChooserParams();

    UE_LOG(LogTemp, Warning, TEXT("Set chooser params successfully!"));

    // Evaluate the Chooser Table to select a montage
    UObject* Result = UChooserFunctionLibrary::EvaluateChooser(this, ChooserTable, UAnimMontage::StaticClass());

    if (Result)
    {
        UAnimMontage* SelectedMontage = Cast<UAnimMontage>(Result);
        if (SelectedMontage)
        {
            animMontage = SelectedMontage; // Assign the dynamically selected montage
            UE_LOG(ACFLog, Log, TEXT("Montage set to: %s"), *SelectedMontage->GetName());
        }
        else
        {
            UE_LOG(ACFLog, Error, TEXT("Chooser result is not a valid AnimMontage!"));
        }
    }
    else
    {
        UE_LOG(ACFLog, Warning, TEXT("No valid montage was selected! Using default montage if set."));
    }

    // Call the base implementation
    Super::Internal_OnActivated(ActionManager, animMontage, ContextString);
}
