#include "PortalLobbyGameState.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "PortalPlayerState.h"

APortalLobbyGameState::APortalLobbyGameState()
{
    bAutoStartActive = false;
    CountdownTimeRemaining = 0.0f;
    SelectedMapName = TEXT("/Game/Maps/PortalDefenseMap");

    bReplicates = true;
    SetReplicateMovement(false);
}

void APortalLobbyGameState::BeginPlay()
{
    Super::BeginPlay();
}

void APortalLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APortalLobbyGameState, LobbyPlayers);
    DOREPLIFETIME(APortalLobbyGameState, SelectedMapName);
    DOREPLIFETIME(APortalLobbyGameState, bAutoStartActive);
    DOREPLIFETIME(APortalLobbyGameState, CountdownTimeRemaining);
}

void APortalLobbyGameState::UpdateLobbyInfo(const TArray<APortalPlayerState*>& Players, const FString& MapName, bool bCountdownActive, float CountdownTime)
{
    if (!HasAuthority()) {
        return;
    }

    // Update players list
    TArray<TObjectPtr<APortalPlayerState>> NewLobbyPlayers;
    for (APortalPlayerState* Player : Players) {
        if (Player) {
            NewLobbyPlayers.Add(Player);
        }
    }

    bool bPlayersChanged = (LobbyPlayers.Num() != NewLobbyPlayers.Num());
    if (!bPlayersChanged) {
        for (int32 i = 0; i < LobbyPlayers.Num(); i++) {
            if (LobbyPlayers[i] != NewLobbyPlayers[i]) {
                bPlayersChanged = true;
                break;
            }
        }
    }

    if (bPlayersChanged) {
        LobbyPlayers = NewLobbyPlayers;
    }

    // Update map selection
    if (SelectedMapName != MapName) {
        SelectedMapName = MapName;
    }

    // Update countdown info
    if (bAutoStartActive != bCountdownActive) {
        bAutoStartActive = bCountdownActive;
    }

    if (CountdownTimeRemaining != CountdownTime) {
        CountdownTimeRemaining = CountdownTime;
    }
}

int32 APortalLobbyGameState::GetReadyPlayerCount() const
{
    int32 ReadyCount = 0;

    for (const TObjectPtr<APortalPlayerState>& Player : LobbyPlayers) {
        if (Player && Player->IsReady()) {
            ReadyCount++;
        }
    }

    return ReadyCount;
}

bool APortalLobbyGameState::AreAllPlayersReady() const
{
    if (LobbyPlayers.Num() == 0) {
        return false;
    }

    for (const TObjectPtr<APortalPlayerState>& Player : LobbyPlayers) {
        if (Player && !Player->IsReady()) {
            return false;
        }
    }

    return true;
}

void APortalLobbyGameState::OnRep_LobbyPlayers()
{
    BroadcastLobbyUpdated();
}

void APortalLobbyGameState::OnRep_SelectedMapName()
{
    OnMapChanged.Broadcast(SelectedMapName);
    BroadcastLobbyUpdated();
}

void APortalLobbyGameState::OnRep_AutoStartActive()
{
    OnCountdownChanged.Broadcast(bAutoStartActive, CountdownTimeRemaining);
    BroadcastLobbyUpdated();
}

void APortalLobbyGameState::OnRep_CountdownTimeRemaining()
{
    OnCountdownChanged.Broadcast(bAutoStartActive, CountdownTimeRemaining);
    BroadcastLobbyUpdated();
}

void APortalLobbyGameState::BroadcastLobbyUpdated()
{
    OnLobbyUpdated.Broadcast();
}