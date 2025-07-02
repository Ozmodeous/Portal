// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFItemTypes.h"
#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"

#include "ACFBowAnimInstance.generated.h"

class AACFWeapon;
class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBowStateChanged, const EBowState&, newState);

/**
 *
 */
UCLASS()
class INVENTORYSYSTEM_API UACFBowAnimInstance : public UAnimInstance {
    GENERATED_BODY()

public:
    UACFBowAnimInstance();

    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

    virtual void NativeInitializeAnimation() override;

    UFUNCTION(BlueprintPure, Category = ACF)
    EBowState GetBowState() const { return bowState; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetBowState(EBowState val);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ReleaseString();

    UFUNCTION(BlueprintCallable, Category = ACF)
    void PullString();

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnBowStateChanged OnBowStateChanged;

protected:
    UFUNCTION(BlueprintPure, Category = ACF)
    AACFWeapon* GetWeaponOwner() const
    {
        return weaponOwner;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    ACharacter* GetItemOwner() const;

    UPROPERTY(EditAnywhere, Category = ACF)
    FName PullSocket;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    AACFWeapon* weaponOwner;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    EBowState bowState;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    float Alpha;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    ACharacter* itemOwner;

      UPROPERTY(BlueprintReadOnly, Category = ACF)
    USkeletalMeshComponent* mainMesh;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    FVector SocketLocation;

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ACF)
    float InterpSpeed;
};
