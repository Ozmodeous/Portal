// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Animation/ACFOverlayLayer.h"
#include "ACFCCFunctionLibrary.h"
#include "Components/ACFCharacterMovementComponent.h"
#include <GameFramework/Pawn.h>

void UACFOverlayLayer::SetReferences()
{
    const APawn* pawn = TryGetPawnOwner();
    if (pawn) {
        MovementComp = Cast<UACFCharacterMovementComponent>(pawn->GetMovementComponent());
        if (!MovementComp) {
            UE_LOG(LogTemp, Error, TEXT("Owner doesn't have ACFCharachterMovement Comp - UACFOverlayLayer::SetReferences!!!!"));

        } else {
            SetMovStance(MovementComp->GetCurrentMovementStance());
        }
    }
}

void UACFOverlayLayer::SetMovStance(const EMovementStance inOverlay)
{
    currentOverlay = inOverlay;
    switch (currentOverlay) {
    case EMovementStance::EIdle:
        targetBlendAlpha = IdleOverlay.BlendAlpha;
        break;
    case EMovementStance::EAiming:
        targetBlendAlpha = AimOverlay.BlendAlpha;
        break;
    case EMovementStance::EBlock:
        targetBlendAlpha = BlockOverlay.BlendAlpha;
        break;
    case EMovementStance::ECustom:
        targetBlendAlpha = CustomOverlay.BlendAlpha;
        break;
    default:
        targetBlendAlpha = IdleOverlay.BlendAlpha;
        break;
    }
}

void UACFOverlayLayer::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    SetReferences();
}

void UACFOverlayLayer::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
    if (!MovementComp) {
        SetReferences();
    } else {
        const EMovementStance newOverlay = MovementComp->GetCurrentMovementStance();
        if (newOverlay != currentOverlay) {
            SetMovStance(newOverlay);
        }
        if (!FMath::IsNearlyEqual(targetBlendAlpha, OverlayBlendAlfa)) {
            OverlayBlendAlfa = FMath::FInterpTo(OverlayBlendAlfa, targetBlendAlpha, DeltaSeconds, 1.f);
        }
    }
}

void UACFOverlayLayer::OnActivated_Implementation()
{
    SetMovStance(currentOverlay);
}
