// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ACFGroupAIComponent.h"
#include "CoreMinimal.h"
#include "Game/ACFDamageType.h"

#include "ACFCompanionGroupAIComponent.generated.h"

/**
 *
 */
UCLASS(ClassGroup = (ACF), Blueprintable, meta = (BlueprintSpawnableComponent))
class AIFRAMEWORK_API UACFCompanionGroupAIComponent : public UACFGroupAIComponent {
    GENERATED_BODY()

public:
    UACFCompanionGroupAIComponent() {};

    UFUNCTION(BlueprintCallable, Category = ACF)
    void AddExpToTheGroup(int32 Exp);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetDespawnIfTooDistanceFromLead(bool bDespawn);

    UFUNCTION(BlueprintPure, Category = ACF)
    bool GetDespawnIfTooDistanceFromLead() const
    {
        return bDespawnIfTooDistanceFromLead;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    float GetMaxDistanceFromLead() const { return MaxDistanceFromLead; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetMaxDistanceFromLead(float val) { MaxDistanceFromLead = val; }

    UFUNCTION(BlueprintPure, Category = ACF)
    float GetUpdateTime() const { return UpdateTime; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetUpdateTime(float val) { UpdateTime = val; }

protected:
    virtual void SetReferences() override;

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "ACF ")
    bool bDespawnIfTooDistanceFromLead = false;

    UPROPERTY(BlueprintReadWrite, meta = (EditCondition = "bDespawnIfTooDistanceFromLead"), EditDefaultsOnly, Category = "ACF ")
    float MaxDistanceFromLead = 3000.f;

    UPROPERTY(BlueprintReadWrite, meta = (EditCondition = "bDespawnIfTooDistanceFromLead "), EditDefaultsOnly, Category = "ACF ")
    float UpdateTime = 1.f;

    virtual void BeginPlay() override;

    void StartUpdateCentroid();

    void StopUpdateCentroid();

private:
    UPROPERTY()
    class AController* contr;

    UFUNCTION()
    void HandleLeadGetHit(const FACFDamageEvent& damage);

    UFUNCTION()
    void HandleLeadHits(const FACFDamageEvent& damage);

    UFUNCTION()
    void HandlePossessedCharacterChanged(const class AACFCharacter* _char);

    UFUNCTION()
    void UpdateCentroid();

    FTimerHandle UpdateTimer;

    bool bUpdating;
};
