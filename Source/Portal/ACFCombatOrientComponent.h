#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "ACFCombatOrientComponent.generated.h"

class AACFCharacter;

UCLASS(ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class PORTAL_API UACFCombatOrientComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UACFCombatOrientComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Orient")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Orient")
    float OrientSpeed = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Orient")
    bool bOnlyOnGround = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Orient")
    float OrientThreshold = 2.0f;

protected:
    UPROPERTY(Replicated)
    FRotator TargetRotation;

    UPROPERTY()
    TObjectPtr<AACFCharacter> OwnerCharacter;

private:
    bool bWasMoving = false;
    bool bShouldOrient = false;
    float LastNetworkUpdate = 0.0f;
    FRotator LastControlRotation;

    void CheckMovementState();
    void UpdateOrientation(float DeltaTime);
    bool ShouldAllowOrientation() const;
    FRotator GetCameraRotation() const;

    UFUNCTION(Server, Unreliable, WithValidation)
    void ServerSetTargetRotation(FRotator NewRotation);
    bool ServerSetTargetRotation_Validate(FRotator NewRotation) { return true; }
    void ServerSetTargetRotation_Implementation(FRotator NewRotation);
};