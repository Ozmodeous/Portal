// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACFItemTypes.h"
#include "Components/ACFArmorSlotComponent.h"

FModularPart::FModularPart(const TObjectPtr<UACFArmorSlotComponent>& inComp)
{
    meshComp = inComp;
    ItemSlot = inComp->GetSlotTag();
};
