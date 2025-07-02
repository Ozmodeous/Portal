#pragma once

#include "ACFCoreTypes.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ACFInteractableInterface.h"
#include "PortalCore.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnergyExtracted, int32, EnergyAmount);

UCLASS()
class PORTAL_API APortalCore : public AActor, public IACFInteractableInterface {
    GENERATED_BODY()

public:
    APortalCore();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // ACF Interactable Interface
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnInteractedByPawn(class APawn* Pawn, const FString& interactionType = "");
    virtual void OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnLocalInteractedByPawn(class APawn* Pawn, const FString& interactionType = "");
    virtual void OnLocalInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnInteractableRegisteredByPawn(class APawn* Pawn);
    virtual void OnInteractableRegisteredByPawn_Implementation(class APawn* Pawn) override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnInteractableUnregisteredByPawn(class APawn* Pawn);
    virtual void OnInteractableUnregisteredByPawn_Implementation(class APawn* Pawn) override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    FText GetInteractableName();
    virtual FText GetInteractableName_Implementation() override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    bool CanBeInteracted(class APawn* Pawn);
    virtual bool CanBeInteracted_Implementation(class APawn* Pawn) override;

    // Health System
    UFUNCTION(BlueprintCallable, Category = "Health")
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable, Category = "Health")
    void ApplyPortalDamage(float DamageAmount, AActor* DamageCauser = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void RestoreHealth(float HealAmount);

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealthPercent() const { return MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDestroyed() const { return bIsDestroyed; }

    // Energy System
    UFUNCTION(BlueprintCallable, Category = "Energy")
    int32 ExtractEnergy();

    UFUNCTION(BlueprintPure, Category = "Energy")
    int32 GetEnergyCapacity() const { return EnergyCapacity; }

    UFUNCTION(BlueprintPure, Category = "Energy")
    float GetEnergyEfficiency() const { return EnergyEfficiency; }

    // Interaction System
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void PlayerInteract(APawn* InteractingPlayer);

    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool CanInteract() const { return !bIsDestroyed && bCanInteract; }

    // Visual Effects
    UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
    void PlayDamageEffect();

    UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
    void PlayEnergyExtractionEffect();

    UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
    void PlayDestructionEffect();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPortalDestroyed OnPortalDestroyed;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEnergyExtracted OnEnergyExtracted;

protected:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PortalMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> InteractionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UWidgetComponent> HealthWidget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UACFTeamManagerComponent> TeamManager;

    // Health Properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", Replicated)
    float MaxHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Health", Replicated)
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
    bool bInvulnerable;

    UPROPERTY(BlueprintReadOnly, Category = "Health")
    bool bIsDestroyed;

    // Energy Properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Energy")
    int32 EnergyCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Energy")
    float EnergyEfficiency;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Energy")
    int32 BaseEnergyExtraction;

    // Interaction Properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    bool bCanInteract;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    float InteractionRange;

    // ACF Interaction Properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACF | Interaction")
    FText InteractableName;

    // Visual Properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    FLinearColor HealthyColor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    FLinearColor DamagedColor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    FLinearColor CriticalColor;

private:
    // Internal Functions
    void UpdateVisualState();
    void HandleDestruction();
    FLinearColor GetHealthBasedColor() const;

    // Networking
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};