#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Game/ACFGameState.h"
#include "PortalDefenseGameState.generated.h"

class APortalCore;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnergyChanged, int32, NewEnergy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPortalHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureProgressChanged, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatrolGuardCountChanged, int32, GuardCount);

UCLASS()
class PORTAL_API APortalDefenseGameState : public AACFGameState {
    GENERATED_BODY()

public:
    APortalDefenseGameState();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // Energy System (kept for portal functionality)
    UFUNCTION(BlueprintCallable, Category = "Energy")
    void AddEnergy(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool SpendEnergy(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Energy")
    void ExtractEnergyFromPortal();

    UFUNCTION(BlueprintPure, Category = "Energy")
    int32 GetCurrentEnergy() const { return CurrentEnergy; }

    UFUNCTION(BlueprintPure, Category = "Energy")
    int32 GetEnergyExtractionRate() const { return EnergyExtractionRate; }

    // Capture System
    UFUNCTION(BlueprintPure, Category = "Capture")
    float GetCaptureProgress() const { return CaptureProgress; }

    UFUNCTION(BlueprintCallable, Category = "Capture")
    void SetCaptureProgress(float Progress);

    UFUNCTION(BlueprintPure, Category = "Capture")
    bool IsCapturing() const { return bIsCapturing; }

    UFUNCTION(BlueprintCallable, Category = "Capture")
    void SetCapturing(bool bCapturing);

    UFUNCTION(BlueprintPure, Category = "Capture")
    int32 GetPlayersInZone() const { return PlayersInCaptureZone; }

    UFUNCTION(BlueprintCallable, Category = "Capture")
    void SetPlayersInZone(int32 PlayerCount);

    // Portal Health
    UFUNCTION(BlueprintPure, Category = "Portal")
    float GetPortalHealthPercent() const;

    UFUNCTION(BlueprintPure, Category = "Portal")
    bool IsPortalDestroyed() const;

    // Enemy Tracking (now patrol guards)
    UFUNCTION(BlueprintCallable, Category = "Guards")
    void RegisterEnemy(APawn* Guard);

    UFUNCTION(BlueprintCallable, Category = "Guards")
    void UnregisterEnemy(APawn* Guard);

    UFUNCTION(BlueprintPure, Category = "Guards")
    int32 GetActiveGuardCount() const { return ActiveGuards.Num(); }

    UFUNCTION(BlueprintPure, Category = "Guards")
    TArray<APawn*> GetActiveGuards() const { return ActiveGuards; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEnergyChanged OnEnergyChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPortalHealthChanged OnPortalHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCaptureProgressChanged OnCaptureProgressChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPatrolGuardCountChanged OnPatrolGuardCountChanged;

protected:
    // Energy Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy", Replicated)
    int32 CurrentEnergy;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    int32 StartingEnergy;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    int32 EnergyExtractionRate;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    float EnergyExtractionInterval;

    // Capture Information
    UPROPERTY(BlueprintReadOnly, Category = "Capture", Replicated)
    float CaptureProgress;

    UPROPERTY(BlueprintReadOnly, Category = "Capture", Replicated)
    bool bIsCapturing;

    UPROPERTY(BlueprintReadOnly, Category = "Capture", Replicated)
    int32 PlayersInCaptureZone;

    // Portal Reference
    UPROPERTY(BlueprintReadOnly, Category = "Portal")
    TObjectPtr<APortalCore> PortalCore;

    UPROPERTY(BlueprintReadOnly, Category = "Portal")
    float LastPortalHealth;

    // Guard Tracking
    UPROPERTY(BlueprintReadOnly, Category = "Guards")
    TArray<TObjectPtr<APawn>> ActiveGuards;

private:
    // Energy Extraction Timer
    FTimerHandle EnergyExtractionTimer;

    // Helper Functions
    void FindPortalCore();
    void StartEnergyExtraction();
    void ExtractEnergyTick();
    void UpdatePortalHealthTracking();

    UFUNCTION()
    void OnGuardDestroyed(AActor* DestroyedActor);

    // Networking
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};