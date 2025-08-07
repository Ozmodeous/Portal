#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "PortalInteractionComponent.generated.h"

class APortalDefenseGameState;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UPortalInteractionComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UPortalInteractionComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Capture Control
    UFUNCTION(BlueprintCallable, Category = "Portal")
    void EnterCaptureZone();

    UFUNCTION(BlueprintCallable, Category = "Portal")
    void ExitCaptureZone();

    UFUNCTION(BlueprintCallable, Category = "Portal")
    void ForceCompletCapture();

    // UI Management
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void UpdateEnergyDisplay(int32 CurrentEnergy);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void UpdateCaptureProgressDisplay(float Progress);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void UpdatePortalHealthDisplay(float HealthPercent);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void UpdateGuardCountDisplay(int32 GuardCount);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void UpdateCaptureStatusDisplay(bool bIsCapturing, int32 PlayersInZone);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void ShowCaptureComplete();

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void ShowCaptureZoneIndicator(bool bShow);

    // Game State Access
    UFUNCTION(BlueprintPure, Category = "Game State")
    APortalDefenseGameState* GetPortalGameState() const;

    // Capture Zone Helpers
    UFUNCTION(BlueprintPure, Category = "Capture")
    bool IsInCaptureZone() const;

    UFUNCTION(BlueprintPure, Category = "Capture")
    float GetDistanceToPortal() const;

    UFUNCTION(BlueprintPure, Category = "Capture")
    float GetCaptureZoneRadius() const;

private:
    // Helper Functions
    void UpdateUI();
    void CheckCaptureZoneStatus();

    // Game State Event Bindings
    UFUNCTION()
    void OnEnergyChanged(int32 NewEnergy);

    UFUNCTION()
    void OnCaptureProgressChanged(float Progress);

    UFUNCTION()
    void OnPortalHealthChanged(float CurrentHealth, float MaxHealth);

    UFUNCTION()
    void OnPatrolGuardCountChanged(int32 GuardCount);

    // Internal State
    UPROPERTY()
    bool bWasInCaptureZone;

    UPROPERTY()
    bool bHasShownCaptureZoneIndicator;
};