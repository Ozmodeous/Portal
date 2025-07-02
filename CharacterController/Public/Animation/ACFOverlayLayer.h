// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAnimLayer.h"
#include "ACFAnimTypes.h"
#include "ACFCCTypes.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"

#include "ACFOverlayLayer.generated.h"

/**
 *
 */
class UAnimSequence;
UCLASS()
class CHARACTERCONTROLLER_API UACFOverlayLayer : public UACFAnimLayer {
    GENERATED_BODY()

protected:
    /*AimOffset Blendpsace applied to have the character point to the target or the camera direction*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | AimOffset")
    UAimOffsetBlendSpace* AimOffset;

    /*Alpha of the AimOffset Blendpsace*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | AimOffset")
    float AimOffsetAlpha = 1.0f;

    /*They can't kept in a map as it seems that accessing TMap in cpp is not thread-safe*/
    /*Overlay to be applied when in Idle*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig IdleOverlay;

    /*Overlay to be applied when Aiming*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig AimOverlay;

    /*Overlay to be applied when Blocking*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig BlockOverlay;

    /*Custom Overlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig CustomOverlay;

    /*Overlay to be applied when Blocking*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACF | Overlay")
    float OverlayBlendAlfa = 0.0f;

    /*Default time to switch between overlays*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    float DefaultSwitchTime = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class UACFCharacterMovementComponent* MovementComp;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    EMovementStance currentOverlay;

    void SetReferences();

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetMovStance(const EMovementStance inOverlay);

    /* begin play */
    virtual void NativeInitializeAnimation() override;

    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

    virtual void OnActivated_Implementation() override;

private:
    float targetBlendAlpha;
};
