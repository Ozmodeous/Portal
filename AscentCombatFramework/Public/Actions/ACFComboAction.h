// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Actions/ACFBaseAction.h"
#include "CoreMinimal.h"

#include "ACFComboAction.generated.h"


/**
 *
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFComboAction : public UACFBaseAction {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE int32 GetComboCounter() const { return CurrentComboIndex; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE bool IsSuccessfulCombo() const { return bSuccesfulCombo; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ResetComboCounter()
    {
        CurrentComboIndex = 0;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ForceComboCounter(int32 val)
    {
        CurrentComboIndex = val;
    }

protected:
    virtual void OnActionStarted_Implementation(const FString& contextString = "") override;

    virtual FName GetMontageSectionName_Implementation() override;

    virtual void OnActionEnded_Implementation() override;

    virtual void OnActionTransition_Implementation(class UACFBaseAction* previousState) override;

    int32 CurrentComboIndex = 0;

    bool bSuccesfulCombo = false;
};
