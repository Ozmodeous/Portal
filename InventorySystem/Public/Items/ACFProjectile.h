// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/ACFEquippableItem.h"
#include <Components/StaticMeshComponent.h>
#include "Interfaces/ACFInteractableInterface.h"
#include "ACMTypes.h"

#include "ACFProjectile.generated.h"

UCLASS()
class INVENTORYSYSTEM_API AACFProjectile : public AACFEquippableItem, public IACFInteractableInterface   {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AACFProjectile();

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UProjectileMovementComponent* GetProjectileMovementComp() { return ProjectileMovementComp; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UStaticMesh* GetStaticMesh() { return MeshComp->GetStaticMesh(); }

    FORCEINLINE class UStaticMeshComponent* GetMeshComponent() const { return MeshComp; };

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UACMCollisionManagerComponent* GetCollisionComp() const { return CollisionComp; }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE bool ShouldBeDroppedOnDeath() const
    {
        return bDroppableWhenAttached;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE float GetDropOnDeathPercentage() const
    {
        return DropRatePercentage;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetupProjectile(class APawn* inOwner);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ActivateDamage();

    UFUNCTION(BlueprintCallable, Category = ACF)
    void AttachToHit(const FHitResult& HitResult, bool inPickable);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void Internal_OnEquipped(class ACharacter* _owner) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class USphereComponent> SphereComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UStaticMeshComponent> MeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UProjectileMovementComponent> ProjectileMovementComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UACMCollisionManagerComponent> CollisionComp;

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "ACF | Projectile")
    float ProjectileLifespan = 5.f;

    /* If this projectile must destroy itself on hit or attach to the actor hit*/
    UPROPERTY(EditDefaultsOnly, Category = "ACF | Projectile")
    EProjectileHitPolicy HitPolicy;

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::AttachOnHit"), Category = "ACF | Projectile")
    float AttachedLifespan = 10.f;

    /* If this is set to true when this projectile is attached to an acfcharacter and
    this character dies, the projectile can be dropped as world item*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::AttachOnHit"), Category = "ACF | Projectile")
    bool bDroppableWhenAttached = true;

    /* When attached to an adversary, the chance that this item will be dropped when it dies*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::AttachOnHit"), Category = "ACF | Projectile")
    float DropRatePercentage = 100.f;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::DestroyOnHit"), Category = "ACF | Projectile")
    FImpactFX ImpactEffect;

    //INTERACTION INTERFACE
    /* called when player interact with object of this class */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnInteractedByPawn(class APawn* Pawn, const FString& interactionType = "");
    virtual void OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    bool CanBeInteracted(class APawn* Pawn);
    virtual bool CanBeInteracted_Implementation(class APawn* Pawn) override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    FText GetInteractableName();
    virtual FText GetInteractableName_Implementation() override;
    //END INTERACTION INTERFACE

private:
    bool bIsFlying;

    UPROPERTY(Replicated)
    bool bPickable;

    void MakeStatic();

    void PlayImpact(const FHitResult& HitResult);

    UFUNCTION()
    void HandleAttackHit(const FHitResult& HitResult);

    UFUNCTION()
    void HandleProjectileHit( UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit );

    bool bImpacted = false;
};
