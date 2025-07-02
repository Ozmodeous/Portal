// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include <GameplayTagContainer.h>
#include <Curves/CurveFloat.h>
#include "Game/ACFTypes.h"
#include "Engine/DeveloperSettings.h"

#include "ACFDeveloperSettings.generated.h"



UCLASS(config = Plugins, Defaultconfig, meta = (DisplayName = "Ascent Combat Settings"))
class ASCENTCOMBATFRAMEWORK_API UACFDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:

	UACFDeveloperSettings();


	/*Default COMBAT TAGS */

	UPROPERTY(EditAnywhere, config, Category = "ACF | Default Tags")
	FGameplayTag DefaultHitState;

	UPROPERTY(EditAnywhere, config, Category = "ACF | Default Tags")
	FGameplayTag DefaultDeathState;


	/*DEFAULT AI TAGS */
	UPROPERTY(EditAnywhere, config, Category = "ACF | Default AI Tags")
	TMap<EAIState, FGameplayTag> DefaultAIStateTags;
	

// 	UPROPERTY(EditAnywhere, config, Category = "ACF | Default Classes")
// 	TSubclassOf<class UACFDamageCalculation> DamageCalculatorClass;
	
};
   