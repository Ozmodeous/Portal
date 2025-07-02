// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include <Curves/CurveFloat.h>
#include <GameplayTagContainer.h>
#include "Items/ACFWorldItem.h"

#include "ACFInventorySettings.generated.h"

UCLASS(config = Plugins, Defaultconfig, meta = (DisplayName = "Ascent Inventory Settings"))
class INVENTORYSYSTEM_API UACFInventorySettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UACFInventorySettings();

    UPROPERTY(EditAnywhere, config, Category = "ACF | Root Tags")
    FGameplayTag ItemtypeTag;

    UPROPERTY(EditAnywhere, config, Category = "ACF | Root Tags")
    FGameplayTag ItemSlotsTag;

    UPROPERTY(EditAnywhere, config, Category = "ACF | Defaults")
    FString DefaultCurrencyName = "Coins";

    UPROPERTY(EditAnywhere, config, Category = "ACF | Defaults")
    TSubclassOf<class AACFWorldItem> WorldItemClass;

 
};
