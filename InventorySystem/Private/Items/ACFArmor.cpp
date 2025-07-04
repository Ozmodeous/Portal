// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Items/ACFArmor.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

AACFArmor::AACFArmor()
{
    ItemInfo.ItemType = EItemType::Armor;
    ItemInfo.Name = FText::FromString("BaseArmor");

    MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Armor Mesh"));

    MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

}

USkinnedAsset* AACFArmor::GetArmorMesh_Implementation(AActor* actorOwner) const
{
    return MeshComp->GetSkinnedAsset();
}

void AACFArmor::BeginPlay()
{
    Super::BeginPlay();
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}
