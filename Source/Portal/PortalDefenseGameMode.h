#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Game/ACFGameMode.h"
#include "PortalDefenseGameMode.generated.h"

class APortalCore;
class APortalDefenseGameState;
class UAIOverlordManager;
class UPortalDefenseSpawner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEnterCaptureZone, APawn*, Player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerExitCaptureZone, APawn*, Player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureProgress, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalCaptured);

UCLASS()
class PORTAL_API APortalDefenseGameMode : public AACFGameMode {
    GENERATED_BODY()

public:
    APortalDefenseGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;

public:
    // Capture System
    UFUNCTION(BlueprintCallable, Category = "Capture System")
    void StartCapture(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Capture System")
    void StopCapture(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Capture System")
    void CompleteCapture();

    UFUNCTION(BlueprintPure, Category = "Capture System")
    bool IsPlayerInCaptureZone(APawn* Player) const;

    UFUNCTION(BlueprintPure, Category = "Capture System")
    float GetCaptureProgress() const { return CaptureProgress; }

    UFUNCTION(BlueprintPure, Category = "Capture System")
    bool IsCaptureActive() const { return bCaptureActive; }

    // Portal and Spawner Access
    UFUNCTION(BlueprintPure, Category = "Portal")
    APortalCore* GetPortalCore() const { return PortalCore; }

    UFUNCTION(BlueprintPure, Category = "Portal")
    UPortalDefenseSpawner* GetPortalSpawner() const { return PortalSpawner; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlayerEnterCaptureZone OnPlayerEnterCaptureZone;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlayerExitCaptureZone OnPlayerExitCaptureZone;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCaptureProgress OnCaptureProgress;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPortalCaptured OnPortalCaptured;

protected:
    // Capture Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Capture System")
    float CaptureZoneRadius = 500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Capture System")
    float TimeToCapture = 60.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Capture System")
    float CaptureProgressDecayRate = 0.5f;

    // Portal Reference
    UPROPERTY(BlueprintReadOnly, Category = "Portal")
    TObjectPtr<APortalCore> PortalCore;

    // Portal Spawner Reference
    UPROPERTY(BlueprintReadOnly, Category = "Portal")
    TObjectPtr<UPortalDefenseSpawner> PortalSpawner;

    // AI Overlord Integration
    UPROPERTY(BlueprintReadOnly, Category = "AI Overlord")
    TObjectPtr<UAIOverlordManager> AIOverlord;

    // Capture State
    UPROPERTY(BlueprintReadOnly, Category = "Capture System")
    bool bCaptureActive;

    UPROPERTY(BlueprintReadOnly, Category = "Capture System")
    float CaptureProgress;

    UPROPERTY(BlueprintReadOnly, Category = "Capture System")
    TArray<TObjectPtr<APawn>> PlayersInZone;

    UPROPERTY(BlueprintReadOnly, Category = "Capture System")
    bool bPortalCaptured;

private:
    // Helper Functions
    void FindPortalCore();
    void UpdateCaptureProgress(float DeltaTime);
    void CheckPlayersInCaptureZone();
};