#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "PortalLobbyGameState.generated.h"

class APortalPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapChanged, const FString&, NewMapName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCountdownChanged, bool, bIsActive, float, TimeRemaining);

UCLASS()
class PORTAL_API APortalLobbyGameState : public AGameStateBase {
    GENERATED_BODY()

public:
    APortalLobbyGameState();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Lobby Information
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdateLobbyInfo(const TArray<APortalPlayerState*>& Players, const FString& MapName, bool bCountdownActive, float CountdownTime);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    TArray<APortalPlayerState*> GetLobbyPlayers() const { return LobbyPlayers; }

    UFUNCTION(BlueprintPure, Category = "Lobby")
    FString GetSelectedMap() const { return SelectedMapName; }

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsCountdownActive() const { return bAutoStartActive; }

    UFUNCTION(BlueprintPure, Category = "Lobby")
    float GetCountdownTime() const { return CountdownTimeRemaining; }

    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetReadyPlayerCount() const;

    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetTotalPlayerCount() const { return LobbyPlayers.Num(); }

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool AreAllPlayersReady() const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnLobbyUpdated OnLobbyUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMapChanged OnMapChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCountdownChanged OnCountdownChanged;

protected:
    // Replicated Properties
    UPROPERTY(ReplicatedUsing = OnRep_LobbyPlayers, BlueprintReadOnly, Category = "Lobby")
    TArray<TObjectPtr<APortalPlayerState>> LobbyPlayers;

    UPROPERTY(ReplicatedUsing = OnRep_SelectedMapName, BlueprintReadOnly, Category = "Lobby")
    FString SelectedMapName;

    UPROPERTY(ReplicatedUsing = OnRep_AutoStartActive, BlueprintReadOnly, Category = "Lobby")
    bool bAutoStartActive;

    UPROPERTY(ReplicatedUsing = OnRep_CountdownTimeRemaining, BlueprintReadOnly, Category = "Lobby")
    float CountdownTimeRemaining;

    // Replication Functions
    UFUNCTION()
    void OnRep_LobbyPlayers();

    UFUNCTION()
    void OnRep_SelectedMapName();

    UFUNCTION()
    void OnRep_AutoStartActive();

    UFUNCTION()
    void OnRep_CountdownTimeRemaining();

private:
    void BroadcastLobbyUpdated();
};