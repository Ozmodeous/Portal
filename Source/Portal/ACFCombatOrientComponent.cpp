#include "ACFCombatOrientComponent.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

UACFCombatOrientComponent::UACFCombatOrientComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f;
    SetIsReplicatedByDefault(true);

    bEnabled = true;
    OrientSpeed = 15.0f;
    bOnlyOnGround = true;
    OrientThreshold = 2.0f;
    bWasMoving = false;
    bShouldOrient = false;
    LastNetworkUpdate = 0.0f;
    TargetRotation = FRotator::ZeroRotator;
    LastControlRotation = FRotator::ZeroRotator;
}

void UACFCombatOrientComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<AACFCharacter>(GetOwner());
    if (!OwnerCharacter) {
        SetComponentTickEnabled(false);
        return;
    }

    LastControlRotation = GetCameraRotation();
}

void UACFCombatOrientComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnabled || !OwnerCharacter)
        return;

    CheckMovementState();

    if (bShouldOrient && ShouldAllowOrientation()) {
        UpdateOrientation(DeltaTime);
    }
}

void UACFCombatOrientComponent::CheckMovementState()
{
    if (!OwnerCharacter)
        return;

    FVector Velocity = OwnerCharacter->GetVelocity();
    bool bIsMoving = Velocity.Size() > 50.0f;
    FRotator CurrentControlRotation = GetCameraRotation();

    // Detect when character stops moving (end of action/attack)
    if (bWasMoving && !bIsMoving) {
        // Check if camera rotated significantly since last stop
        float RotationDiff = FMath::Abs(FRotator::NormalizeAxis(CurrentControlRotation.Yaw - LastControlRotation.Yaw));

        if (RotationDiff > OrientThreshold) {
            if (GetOwnerRole() == ROLE_AutonomousProxy || GetOwnerRole() == ROLE_Authority) {
                TargetRotation = FRotator(0.0f, CurrentControlRotation.Yaw, 0.0f);

                if (GetOwnerRole() == ROLE_AutonomousProxy) {
                    float CurrentTime = GetWorld()->GetTimeSeconds();
                    if (CurrentTime - LastNetworkUpdate > 0.1f) {
                        ServerSetTargetRotation(TargetRotation);
                        LastNetworkUpdate = CurrentTime;
                    }
                }
            }
            bShouldOrient = true;
        }
        LastControlRotation = CurrentControlRotation;
    }

    bWasMoving = bIsMoving;
}

void UACFCombatOrientComponent::UpdateOrientation(float DeltaTime)
{
    FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
    FRotator NewRotation = UKismetMathLibrary::RInterpTo(
        CurrentRotation,
        TargetRotation,
        DeltaTime,
        OrientSpeed);

    OwnerCharacter->SetActorRotation(NewRotation);

    // Stop orienting when close enough
    if (FMath::Abs(FRotator::NormalizeAxis(NewRotation.Yaw - TargetRotation.Yaw)) < 2.0f) {
        bShouldOrient = false;
    }
}

bool UACFCombatOrientComponent::ShouldAllowOrientation() const
{
    if (bOnlyOnGround && OwnerCharacter) {
        UACFCharacterMovementComponent* Movement = OwnerCharacter->GetACFCharacterMovementComponent();
        if (Movement && !Movement->IsMovingOnGround())
            return false;
    }
    return true;
}

FRotator UACFCombatOrientComponent::GetCameraRotation() const
{
    if (OwnerCharacter && OwnerCharacter->GetController()) {
        return OwnerCharacter->GetController()->GetControlRotation();
    }
    return FRotator::ZeroRotator;
}

void UACFCombatOrientComponent::ServerSetTargetRotation_Implementation(FRotator NewRotation)
{
    TargetRotation = NewRotation;
}

void UACFCombatOrientComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UACFCombatOrientComponent, TargetRotation);
}