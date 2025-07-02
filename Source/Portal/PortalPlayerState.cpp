#include "PortalPlayerState.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"

APortalPlayerState::APortalPlayerState()
{
    bIsReady = false;
    bIsInLobby = false;
    SelectedCharacterClass = 0;
    TeamID = 0;
    PlayerDisplayName = TEXT("Player");

    // Enable replication
    bReplicates = true;
    SetReplicateMovement(false);
}

void APortalPlayerState::BeginPlay()
{
    Super::BeginPlay();

    // Set default display name if empty
    if (PlayerDisplayName.IsEmpty() && !GetPlayerName().IsEmpty()) {
        PlayerDisplayName = GetPlayerName();
    }
}

void APortalPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APortalPlayerState, bIsReady);
    DOREPLIFETIME(APortalPlayerState, PlayerDisplayName);
    DOREPLIFETIME(APortalPlayerState, SelectedCharacterClass);
    DOREPLIFETIME(APortalPlayerState, TeamID);
}

void APortalPlayerState::ServerSetReady_Implementation(bool bNewReady)
{
    if (bIsReady != bNewReady) {
        bIsReady = bNewReady;

        UE_LOG(LogTemp, Log, TEXT("Player %s ready state changed to: %s"),
            *GetPlayerName(), bIsReady ? TEXT("Ready") : TEXT("Not Ready"));
    }
}

void APortalPlayerState::ServerSetPlayerDisplayName_Implementation(const FString& NewDisplayName)
{
    if (!NewDisplayName.IsEmpty() && PlayerDisplayName != NewDisplayName) {
        PlayerDisplayName = NewDisplayName;
        SetPlayerName(NewDisplayName);

        UE_LOG(LogTemp, Log, TEXT("Player display name changed to: %s"), *PlayerDisplayName);
    }
}

void APortalPlayerState::ServerSetSelectedCharacterClass_Implementation(int32 CharacterClassIndex)
{
    if (SelectedCharacterClass != CharacterClassIndex) {
        SelectedCharacterClass = CharacterClassIndex;

        UE_LOG(LogTemp, Log, TEXT("Player %s selected character class: %d"),
            *GetPlayerName(), SelectedCharacterClass);
    }
}

void APortalPlayerState::ServerSetTeamID_Implementation(int32 NewTeamID)
{
    if (TeamID != NewTeamID) {
        TeamID = NewTeamID;

        UE_LOG(LogTemp, Log, TEXT("Player %s assigned to team: %d"),
            *GetPlayerName(), TeamID);
    }
}

void APortalPlayerState::OnRep_IsReady()
{
    OnPlayerReadyStateChanged.Broadcast(bIsReady);

    UE_LOG(LogTemp, Log, TEXT("OnRep_IsReady: Player %s is now %s"),
        *GetPlayerName(), bIsReady ? TEXT("Ready") : TEXT("Not Ready"));
}