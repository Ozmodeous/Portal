// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFArmorSlotComponent.h"

UACFArmorSlotComponent::UACFArmorSlotComponent()
{
    SetIsReplicatedByDefault(true);
}

void UACFArmorSlotComponent::ResetSlotToEmpty()
{
    if (EmptySlotMesh && hasEmptyVersion) {
        SetSkinnedAssetAndUpdate(EmptySlotMesh);
    } else {
        SetVisibility(false);
    }
}

void UACFArmorSlotComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UACFArmorSlotComponent::InitializeComponent()
{
    Super::InitializeComponent();
    EmptySlotMesh = GetSkeletalMeshAsset();
    hasEmptyVersion = true;
}

