// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Actions/ACFComboAction.h"
#include "RootMotionModifier.h"
#include "Game/ACFTypes.h"
#include "ACFAttackAction.generated.h"

/**
 * 
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFAttackAction : public UACFComboAction
{
	GENERATED_BODY()

public: 

	UACFAttackAction();

protected:

	virtual void OnActionStarted_Implementation(const FString& contextString = "") override;

	virtual void OnActionEnded_Implementation() override;

	virtual void OnSubActionStateEntered_Implementation() override;

	virtual void OnSubActionStateExited_Implementation() override;

	virtual FTransform GetWarpTransform_Implementation() override;

	virtual USceneComponent* GetWarpTargetComponent_Implementation() override;

	virtual void OnTick_Implementation(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = ACF)
	EDamageActivationType DamageToActivate;

	UPROPERTY(EditDefaultsOnly, Category = ACF)
	TArray<FName> TraceChannels;


	UPROPERTY(EditDefaultsOnly, Category = "ACF| Warp")
	bool bCheckWarpConditions = true;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bCheckWarpConditions), Category = "ACF| Warp")
	float maxWarpDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bCheckWarpConditions), Category = "ACF| Warp")
	float minWarpDistance = 10.f;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bCheckWarpConditions), Category = "ACF| Warp")
	float maxWarpAngle = 270.f;

	UPROPERTY(EditDefaultsOnly, Category = "ACF| Warp")
    bool bContinuousUpdate = true;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bContinuousUpdate), Category = "ACF| Warp")
    float WarpMagnetismStrength = 1.0f;

	bool TryGetTransform(FTransform& outTranform) const;

private :

	FTransform warpTrans;
	TObjectPtr<USceneComponent> currentTargetComp;

	EMontageReproductionType storedReproType;
};
