// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chooser.h"
#include "Actions/ACFBaseAction.h"
#include "ACFChooserAction.generated.h"

/**
 * A child action class that uses a Chooser Table to dynamically select a montage.
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class ASCENTCOMBATFRAMEWORK_API UACFChooserAction : public UACFBaseAction
{
    GENERATED_BODY()

public:
    UACFChooserAction();

    /** Reference to the Chooser Table */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
    UChooserTable* ChooserTable;

    /** Function to set parameters dynamically in Blueprint */
    UFUNCTION(BlueprintNativeEvent, Category = "Chooser")
    void SetChooserParams();

protected:
    virtual void SetChooserParams_Implementation();

    /**
     * Override to dynamically assign the montage before execution.
     */
    virtual void Internal_OnActivated(UACFActionsManagerComponent* ActionManager, UAnimMontage* InAnimMontage, const FString& ContextString) override;

    /** Helper function to evaluate the Chooser Table */
    UAnimMontage* EvaluateChooserTable();
};