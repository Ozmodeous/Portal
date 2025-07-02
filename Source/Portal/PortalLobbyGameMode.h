#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "PortalLobbyGameMode.generated.h"

class APortalPlayerState;
class APortalLobbyGameState;
class ALobbyPlayerController;

UCLASS()
class PORTAL_API APortalLobbyGameMode : public AGameModeBase {
    GENERATED_BODY()

public:
    APortalLobbyGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void RestartPlayer(AController* NewPlayer) override;

public:
    // Lobby Management
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void StartGame();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ChangeMap(const FString& NewMapName);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool CanStartGame() const;

    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetReadyPlayerCount() const;

    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetTotalPlayerCount() const;

    UFUNCTION(BlueprintPure, Category = "Lobby")
    TArray<APortalPlayerState*> GetAllPlayerStates() const;

    // Server Settings
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetRequiredReadyPlayers(int32 RequiredPlayers);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetRequiredReadyPlayers() const { return RequiredReadyPlayers; }

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetAutoStartCountdown(float CountdownTime);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CancelAutoStart();

    // Available Maps
    UFUNCTION(BlueprintPure, Category = "Lobby")
    TArray<FString> GetAvailableMaps() const { return AvailableMaps; }

    UFUNCTION(BlueprintPure, Category = "Lobby")
    FString GetCurrentSelectedMap() const { return CurrentSelectedMap; }

protected:
    // Lobby Settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
    int32 RequiredReadyPlayers;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
    int32 MaxPlayers;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
    float AutoStartCountdown;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
    bool bRequireAllPlayersReady;

    // Available Maps
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
    TArray<FString> AvailableMaps;

    UPROPERTY(BlueprintReadOnly, Category = "Maps")
    FString CurrentSelectedMap;

    // Auto Start Timer
    UPROPERTY(BlueprintReadOnly, Category = "Lobby")
    float CurrentCountdownTime;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby")
    bool bAutoStartActive;

private:
    FTimerHandle AutoStartTimerHandle;
    FTimerHandle ReadyCheckTimerHandle;

    void CheckReadyStatus();
    void StartAutoStartCountdown();
    void HandleAutoStart();
    void BroadcastLobbyUpdate();

    UFUNCTION()
    void OnAutoStartTimer();

    UFUNCTION()
    void OnReadyCheckTimer();
};