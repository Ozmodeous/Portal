// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Actions/ACFBaseAction.h"
#include "CoreMinimal.h"

#include "ACFSustainedAction.generated.h"

/**
 *
 */
UCLASS()
class ACTIONSSYSTEM_API UACFSustainedAction : public UACFBaseAction {
    GENERATED_BODY()

public:
    UACFSustainedAction();

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ReleaseAction();

    UFUNCTION(BlueprintCallable, Category = ACF)
    virtual void PlayActionSection(FName sectionName);

    UFUNCTION(BlueprintPure, Category = ACF)
    float GetActionElapsedTime() const;

    UFUNCTION(BlueprintPure, Category = ACF)
    ESustainedActionState GetActionState() const {
        return ActionState;
    }

protected:
    UPROPERTY(EditDefaultsOnly, Category = ACF)
    FName FinalSectionName = "End";

    /*Called when release action is triggered during the execution*/
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnActionReleased(float elapsedTimeSeconds);
    virtual void OnActionReleased_Implementation(float elapsedTimeSeconds);

    float startTime;

    ESustainedActionState ActionState;

    virtual void Internal_OnActivated(class UACFActionsManagerComponent* actionmanger,
        class UAnimMontage* inAnimMontage, const FString& contextString) override;

    virtual void Internal_OnDeactivated() override;
};
