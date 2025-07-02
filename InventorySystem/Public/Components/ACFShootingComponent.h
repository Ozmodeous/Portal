// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFEquipmentComponent.h"
#include "ACFItemTypes.h"
#include "ACMTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Items/ACFWeapon.h"
#include <Components/MeshComponent.h>

#include "ACFShootingComponent.generated.h"

struct FImpactFX;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrentAmmoChanged, int32, currentAmmoInMagazine, int32, totalAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShoot);

UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFShootingComponent : public UActorComponent {
    GENERATED_BODY()

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

     /*Socket in the provided mesh the projectile starts from*/
    UPROPERTY(EditAnywhere, Category = "ACF")
    FName ProjectileStartSocket = "ProjectileStart";

     /*Shooting effect*/
    UPROPERTY(EditAnywhere, Category = "ACF")
    struct FImpactFX ShootingEffect;

    /*If shooting should check for Ammos*/
    UPROPERTY(EditDefaultsOnly, Category = "ACF | Ammo")
    bool bConsumeAmmo = true;

     /*The equipment slot that will be checked to consume Ammos when shooting*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "ACF | Ammo")
    FGameplayTag AmmoSlot;

    /*Projectiles allowed to be used with this component*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "ACF | Ammo")
    TArray<TSubclassOf<class AACFProjectile>> AllowedProjectiles;

    /*If this is set to true means that this weapon will need to be Reload every time his ammo in
    magazine are 0 to continue shooting. */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "ACF | Ammo")
    bool bUseMagazine = false;

    /*The number of projectile to be shot before reloading*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseMagazine"), Category = "ACF | Ammo")
    int32 AmmoMagazine = 10;

    
    /*The default projectile class to be shot*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo == false"), Category = "ACF | Ammo")
    TSubclassOf<class AACFProjectile> ProjectileClassBP;

    /*Speed at wich the projectile is shot*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | Projectile Shoot Config")
    float ProjectileShotSpeed;

    /*Radius of the shooting trace. 0 means linetrace*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | SwipeTrace Shoot Config")
    float ShootRadius = 1.f;

    /*Distance at which the trace is done*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | SwipeTrace Shoot Config")
    float ShootRange = 3500.f;

public:
    // Sets default values for this component's properties
    UACFShootingComponent();

    /* Removes one ammo from the current magazine */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void RemoveAmmo();

    /* Initializes the shooting component on the server, associating it with an owner Pawn and a shooting mesh */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = ACF)
    void SetupShootingComponent(class APawn* inOwner, class UMeshComponent* inMesh);

    /* Reinitializes the shooting component with a new owner, mesh, start socket, and shooting effects */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void ReinitializeShootingComponent(class APawn* inOwner, class UMeshComponent* inMesh, FName inStartSocket, const FImpactFX& inShootingFX)
    {
        shootingMesh = inMesh;
        characterOwner = inOwner;
        ProjectileStartSocket = inStartSocket;
        ShootingEffect = inShootingFX;
    }

    /* Returns the projectile speed */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE float GetProjectileSpeed() const { return ProjectileShotSpeed; }

    /* Fires a projectile at a specified target with optional deviation, charge, and projectile override */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void ShootAtActor(const AActor* target, float randomDeviation = 5.f, float charge = 1.f, TSubclassOf<class AACFProjectile> projectileOverride = nullptr, const FName socketOverride = NAME_None);

    /* Fires a projectile in a specific direction with optional charge and projectile override */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void ShootAtDirection(const FRotator& direction, float charge = 1.f, TSubclassOf<class AACFProjectile> projectileOverride = nullptr, const FName socketOverride = NAME_None);

    /* Shoots a projectile based on shooting type and target type */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void Shoot(APawn* SourcePawn, EShootingType type, EShootTargetType targetType, float charge = 1.f, TSubclassOf<class AACFProjectile> projectileOverride = nullptr);

    /* Performs a swipe trace shooting action in a given direction with a delay */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SwipeTraceShootAtDirection(const FVector& start, const FVector& direction, float shootDelay = 0.1f);

    /* Reduces the ammo magazine count by a specified amount */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = ACF)
    void ReduceAmmoMagazine(int32 amount = 1);

    /* Reloads the weapon, refilling the magazine.
     * If bTryToEquipAmmo is true, it will attempt to equip ammo from inventory.
     */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = ACF)
    void Reload(bool bTryToEquipAmmo = true);

    /* Attempts to equip ammo from the inventory */
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool TryEquipAmmoFromInventory();

    /* Checks if the weapon can shoot */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool CanShoot() const;

    /* Checks if the specified projectile type can be used */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool CanUseProjectile(const TSubclassOf<AACFProjectile>& projectileClass) const;

    /* Checks if the weapon needs a reload */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool NeedsReload() const;

    /* Checks if the magazine is full */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool IsFullMagazine() const
    {
        return currentMagazine == AmmoMagazine;
    }

    /* Attempts to retrieve an allowed ammo type from the inventory */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool TryGetAllowedAmmoFromInventory(FInventoryItem& outAmmoSlot) const;

    /* Returns the world position of the shooting socket */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FVector GetShootingSocketPosition() const
    {
        return shootingMesh->GetSocketLocation(ProjectileStartSocket);
    }

    /* Returns the name of the socket from which projectiles are fired */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FName GetProjectileStartSocketName() const
    {
        return ProjectileStartSocket;
    }

    /* Sets the socket name from which projectiles should be fired */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetProjectileStartSocketName(FName newSocket)
    {
        ProjectileStartSocket = newSocket;
    }

    /* Returns the mesh component associated with shooting */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE UMeshComponent* GetShootingMesh() const
    {
        return shootingMesh;
    }

    /* Returns the list of allowed projectile types */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE TArray<TSubclassOf<AACFProjectile>> GetAllowedProjectiles() const
    {
        return AllowedProjectiles;
    }

    /* Returns the total magazine capacity */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE int32 GetAmmoMagazine() const
    {
        return AmmoMagazine;
    }

    /* Returns the total count of equipped ammo */
    UFUNCTION(BlueprintPure, Category = ACF)
    int32 GetTotalEquippedAmmoCount() const;

    /* Returns the current amount of ammo in the magazine */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE int32 GetCurrentAmmoInMagazine() const
    {
        return currentMagazine;
    }

    /* Returns the total ammo count available */
    UFUNCTION(BlueprintPure, Category = ACF)
    int32 GetTotalAmmoCount() const;

    /* Delegate triggered when the current ammo count changes */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnCurrentAmmoChanged OnCurrentAmmoChanged;

    /* Delegate triggered when a projectile is shot */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnShoot OnProjectileShoot;

    /* Plays a muzzle effect when a shot is fired */
    UFUNCTION(NetMulticast, Reliable, Category = ACF)
    void PlayMuzzleEffect();

    /* Checks whether the weapon uses a magazine system */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool GetUseMagazine() const { return bUseMagazine; }

    /* Attempts to retrieve an ammo slot from equipped items */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool TryGetAmmoSlot(FEquippedItem& outSlot) const;

    /* Enables or disables the magazine system */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetUseMagazine(bool val) { bUseMagazine = val; }

private:
    UPROPERTY(Replicated)
    TObjectPtr<UMeshComponent> shootingMesh;

    UPROPERTY(Replicated)
    TObjectPtr<APawn> characterOwner;

    UPROPERTY(ReplicatedUsing = OnRep_currentMagazine)
    int32 currentMagazine;

    UFUNCTION()
    void OnRep_currentMagazine();

    UFUNCTION()
    void FinishSwipe(const FVector& start, const FVector& direction);

    void Internal_Shoot(const FTransform& spawnTransform, const FVector& ShotDirection, float charge, TSubclassOf<class AACFProjectile> inProjClass);

    UFUNCTION(NetMulticast, Reliable)
    void Internal_SetupComponent(class APawn* inOwner, class UMeshComponent* inMesh);

    class UACFEquipmentComponent* TryGetEquipment() const;

    TSubclassOf<AACFItem> GetBestProjectileToShoot() const;
    FTimerHandle newTimer;
    bool bSwipeShooting;
};
