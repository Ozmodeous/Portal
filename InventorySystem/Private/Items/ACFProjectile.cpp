// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Items/ACFProjectile.h"
#include "ACMCollisionManagerComponent.h"
#include "Components/ACFEquipmentComponent.h"
#include "Components/ACFTeamManagerComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/ACFEntityInterface.h"
#include "Interfaces/ACFInteractableInterface.h"
#include "Items/ACFRangedWeapon.h"
#include <Components/SceneComponent.h>
#include <Components/StaticMeshComponent.h>
#include <Engine/EngineTypes.h>
#include <GameFramework/Actor.h>
#include <GameFramework/ProjectileMovementComponent.h>
#include <Kismet/GameplayStatics.h>
#include <Particles/ParticleSystemComponent.h>
#include "ACMCollisionsFunctionLibrary.h"
#include <Net/UnrealNetwork.h>

// Sets default values
AACFProjectile::AACFProjectile()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
    // Create A Default Root Component as a container
    SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("RootComp"));
    SetRootComponent(SphereComp);

    CollisionComp = CreateDefaultSubobject<UACMCollisionManagerComponent>(TEXT("Collision Manager Comp"));
    // Attach Mesh component to root component
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMeshComp"));
    MeshComp->SetupAttachment(SphereComp);
    MeshComp->SetCollisionProfileName(TEXT("NoCollision"));
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bReplicates = true;

    SetReplicateMovement(true);
    ItemInfo.Name = FText::FromString("Base Projectile");
    // Use a ProjectileMovementComponent to govern this projectile's movement
    ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));

    ProjectileMovementComp->UpdatedComponent = SphereComp;
    ProjectileMovementComp->InitialSpeed = 4000.f;
    ProjectileMovementComp->MaxSpeed = 5000.0f;
    ProjectileMovementComp->bRotationFollowsVelocity = true;
    ProjectileMovementComp->bShouldBounce = false;
    ProjectileMovementComp->bAutoActivate = false;

    bIsFlying = false;
    bPickable = false;
    ItemInfo.ItemType = EItemType::Projectile;
    SphereComp->SetSphereRadius(4.0f);
    HitPolicy = EProjectileHitPolicy::DestroyOnHit;
}

// Called when the game starts or when spawned
void AACFProjectile::BeginPlay()
{
    Super::BeginPlay();
    if (CollisionComp) {
        CollisionComp->SetActorOwner(ItemOwner);
        CollisionComp->SetupCollisionManager(MeshComp);
    }
    if (SphereComp  && !SphereComp->OnComponentHit.IsAlreadyBound(this, &AACFProjectile::HandleProjectileHit) ) {
        SphereComp->OnComponentHit.AddDynamic(this, &AACFProjectile::HandleProjectileHit);
    }
    if (bIsFlying) {
         ProjectileMovementComp->SetActive(true);
    }
    
}

void AACFProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (CollisionComp) {
        CollisionComp->StopAllTraces();
    }
}

void AACFProjectile::Internal_OnEquipped(class ACharacter* _owner)
{
    Super::Internal_OnEquipped(_owner);
    bPickable = false;
     SetActorHiddenInGame(true);
     SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AACFProjectile::OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType /*= ""*/)
{
    if (Pawn) {
        UACFEquipmentComponent* equipComp = Pawn->GetComponentByClass<UACFEquipmentComponent>();
        if (equipComp) {
            equipComp->AddItemToInventoryByClass(GetClass(), 1);
            Destroy();
        }
    }
   
}

bool AACFProjectile::CanBeInteracted_Implementation(class APawn* Pawn)
{
    return bPickable;
}

FText AACFProjectile::GetInteractableName_Implementation()
{
    return GetItemName();
}

void AACFProjectile::SetupProjectile(class APawn* inOwner)
{
    if (inOwner) {
        bIsFlying = true;
        ItemOwner = inOwner;
        SetLifeSpan(ProjectileLifespan);
       
    } else {
        bIsFlying = false;
    }
}

void AACFProjectile::ActivateDamage()
{
    if (CollisionComp) {
        CollisionComp->SetActorOwner(ItemOwner);
        CollisionComp->SetupCollisionManager(MeshComp);

        const bool bImplements = ItemOwner->GetClass()->ImplementsInterface(UACFEntityInterface::StaticClass());
        if (bImplements) {
            const ETeam combatTeam = IACFEntityInterface::Execute_GetEntityCombatTeam(ItemOwner);
            const AGameStateBase* gameState = UGameplayStatics::GetGameState(this);
            const UACFTeamManagerComponent* teamManager = gameState->FindComponentByClass<UACFTeamManagerComponent>();
            if (teamManager) {
                CollisionComp->AddCollisionChannels(teamManager->GetEnemiesCollisionChannels(combatTeam));
            } else {
                UE_LOG(LogTemp, Error, TEXT("NO  TEAM MANAGER MANAGER ON GAMESTATE! - AACFProjectile"));
            }
        }
        bImpacted = false;
        CollisionComp->StartAllTraces();
        CollisionComp->OnCollisionDetected.AddDynamic(this, &AACFProjectile::HandleAttackHit);
    }
}

void AACFProjectile::MakeStatic()
{
    ProjectileMovementComp->SetActive(false);
    ProjectileMovementComp->Velocity = FVector::ZeroVector;
    SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));
    if (CollisionComp) {
        CollisionComp->StopAllTraces();
    }
}

void AACFProjectile::PlayImpact(const FHitResult& HitResult)
{
    if (!bImpacted && CollisionComp) {
        UACMCollisionsFunctionLibrary::PlayImpactEffect(CollisionComp->GetFirstTrace().DamageTypeClass, HitResult.PhysMaterial.Get(), HitResult.Location, this);

        const FImpactFX impact = FImpactFX(ImpactEffect, HitResult.ImpactPoint);
        UACMCollisionsFunctionLibrary::PlayEffectLocally(impact, this);
        bImpacted = true;
    }
}

void AACFProjectile::HandleAttackHit(const FHitResult& HitResult)
{
    switch (HitPolicy) {
    case EProjectileHitPolicy::DestroyOnHit:
        SetLifeSpan(.1f);
        break;
    case EProjectileHitPolicy::AttachOnHit:
        AttachToHit(HitResult, false);
        break;
    default:
        break;
    }
    PlayImpact(HitResult);
}

void AACFProjectile::HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    switch (HitPolicy) {
    case EProjectileHitPolicy::DestroyOnHit:

        SetLifeSpan(.1f);
        break;
    case EProjectileHitPolicy::AttachOnHit:
        AttachToHit(Hit, true);

        break;
    default:
        break;
    }
    PlayImpact(Hit);
}

void AACFProjectile::AttachToHit(const FHitResult& HitResult, bool inPickable)
{
    ACharacter* damagedActor = Cast<ACharacter>(HitResult.GetActor());
    //   const FVector AttachLocation = GetActorLocation() + (GetActorForwardVector() * PenetrationLevel) + (GetActorUpVector() * PenetrationLevel);
    const FRotator AttachRotation = GetActorRotation();
    bPickable = inPickable;
    MakeStatic();
    if (damagedActor) {
        AttachToComponent(damagedActor->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HitResult.BoneName);
        SetActorRotation(AttachRotation);
    } else {
        AttachToComponent(HitResult.Component.Get(), FAttachmentTransformRules::KeepWorldTransform);
    }
    SetActorRotation(GetActorRotation());
    SetLifeSpan(AttachedLifespan);
    CollisionComp->StopAllTraces();
    // Destroy();
}

void AACFProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AACFProjectile, bPickable);
}