// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Actions/ACFBaseAction.h"
#include "CoreMinimal.h"

#include "ACFDirectionalDodgeAction.generated.h"

/**
 *
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFDirectionalDodgeAction : public UACFBaseAction {
    GENERATED_BODY()

protected:
    UACFDirectionalDodgeAction();

    virtual void OnActionStarted_Implementation(const FString& contextString = "") override;

    virtual void OnActionEnded_Implementation() override;

    virtual FName GetMontageSectionName_Implementation() override;

    virtual void OnSubActionStateEntered_Implementation() override;

    virtual void OnSubActionStateExited_Implementation() override;

    virtual FTransform GetWarpTransform_Implementation() override;

    UFUNCTION(BlueprintPure, Category = ACF)
    EACFDirection GetDodgeDirection() const
    {
        return finalDirection;
    }

     /*The montage section name to select the animation be played depending on the dodge direction*/
    UPROPERTY(EditDefaultsOnly, Category = ACF)
    TMap<EACFDirection, FName> DodgeDirectionToMontageSectionMap;

     /*The length of the dodge in centimeters*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bShouldWarp), Category = "ACF| Warp")
    float DodgeLength = 600.f;

    /*The direction of the dodge when no directional input is found*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bShouldWarp), Category = "ACF| Warp")
    EACFDirection defaultDodgeDirection;

    /*If the dodge shuould be constrained around current target (useful for lock on dodges)*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bShouldWarp), Category = "ACF| Warp")
    bool bConstraintRotationAroundTarged;

private:
    FVector dodgeDirection;

    EACFDirection finalDirection;

    TObjectPtr<AActor> TryGetCurrentTarget();

public:

    UFUNCTION(BlueprintPure, Category = ACF)
    float GetDodgeLength() const { return DodgeLength; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetDodgeLength(float val) { DodgeLength = val; }


};
