#include "PortalLobbyGameMode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LobbyPlayerController.h"
#include "PortalGameInstance.h"
#include "PortalLobbyGameState.h"
#include "PortalPlayerState.h"

APortalLobbyGameMode::APortalLobbyGameMode()
{
    PlayerStateClass = APortalPlayerState::StaticClass();
    GameStateClass = APortalLobbyGameState::StaticClass();
    PlayerControllerClass = ALobbyPlayerController::StaticClass();

    RequiredReadyPlayers = 1;
    MaxPlayers = 4;
    AutoStartCountdown = 10.0f;
    bRequireAllPlayersReady = false;
    CurrentCountdownTime = 0.0f;
    bAutoStartActive = false;

    // Setup available maps
    AvailableMaps.Add(TEXT("/Game/Maps/PortalDefenseMap"));
    AvailableMaps.Add(TEXT("/Game/Maps/TestArena"));
    AvailableMaps.Add(TEXT("/Game/Maps/CoopDefense"));

    CurrentSelectedMap = AvailableMaps.Num() > 0 ? AvailableMaps[0] : TEXT("/Game/Maps/PortalDefenseMap");

    bUseSeamlessTravel = true;
}

void APortalLobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Start periodic ready check
    GetWorldTimerManager().SetTimer(ReadyCheckTimerHandle, this, &APortalLobbyGameMode::OnReadyCheckTimer, 1.0f, true);
}

void APortalLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (APortalPlayerState* PortalPlayerState = NewPlayer->GetPlayerState<APortalPlayerState>()) {
        PortalPlayerState->SetInLobby(true);
        UE_LOG(LogTemp, Warning, TEXT("Player %s joined lobby"), *PortalPlayerState->GetPlayerName());
    }

    BroadcastLobbyUpdate();
}

void APortalLobbyGameMode::Logout(AController* Exiting)
{
    if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting)) {
        if (APortalPlayerState* PortalPlayerState = ExitingPC->GetPlayerState<APortalPlayerState>()) {
            UE_LOG(LogTemp, Warning, TEXT("Player %s left lobby"), *PortalPlayerState->GetPlayerName());
        }
    }

    Super::Logout(Exiting);
    BroadcastLobbyUpdate();

    // Cancel auto start if player leaves
    if (bAutoStartActive) {
        CancelAutoStart();
    }
}

void APortalLobbyGameMode::RestartPlayer(AController* NewPlayer)
{
    // Don't spawn pawns in lobby
}

void APortalLobbyGameMode::StartGame()
{
    if (!CanStartGame()) {
        UE_LOG(LogTemp, Warning, TEXT("Cannot start game - not enough ready players"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Starting game on map: %s"), *CurrentSelectedMap);

    // Use Game Instance to start the game
    if (UPortalGameInstance* GameInstance = Cast<UPortalGameInstance>(GetGameInstance())) {
        GameInstance->StartGame(CurrentSelectedMap);
    } else {
        // Fallback to direct server travel
        GetWorld()->ServerTravel(CurrentSelectedMap);
    }
}

void APortalLobbyGameMode::ChangeMap(const FString& NewMapName)
{
    if (AvailableMaps.Contains(NewMapName)) {
        CurrentSelectedMap = NewMapName;
        BroadcastLobbyUpdate();
        UE_LOG(LogTemp, Log, TEXT("Map changed to: %s"), *CurrentSelectedMap);
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Invalid map name: %s"), *NewMapName);
    }
}

bool APortalLobbyGameMode::CanStartGame() const
{
    int32 ReadyCount = GetReadyPlayerCount();
    int32 TotalCount = GetTotalPlayerCount();

    if (TotalCount == 0) {
        return false;
    }

    if (bRequireAllPlayersReady) {
        return ReadyCount == TotalCount && TotalCount >= RequiredReadyPlayers;
    } else {
        return ReadyCount >= RequiredReadyPlayers;
    }
}

int32 APortalLobbyGameMode::GetReadyPlayerCount() const
{
    int32 ReadyCount = 0;

    for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
        if (APlayerController* PC = Iterator->Get()) {
            if (APortalPlayerState* PortalPlayerState = PC->GetPlayerState<APortalPlayerState>()) {
                if (PortalPlayerState->IsReady()) {
                    ReadyCount++;
                }
            }
        }
    }

    return ReadyCount;
}

int32 APortalLobbyGameMode::GetTotalPlayerCount() const
{
    // Count players manually to avoid const issues
    int32 PlayerCount = 0;
    for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
        if (Iterator->Get()) {
            PlayerCount++;
        }
    }
    return PlayerCount;
}

TArray<APortalPlayerState*> APortalLobbyGameMode::GetAllPlayerStates() const
{
    TArray<APortalPlayerState*> PlayerStates;

    for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
        if (APlayerController* PC = Iterator->Get()) {
            if (APortalPlayerState* PortalPlayerState = PC->GetPlayerState<APortalPlayerState>()) {
                PlayerStates.Add(PortalPlayerState);
            }
        }
    }

    return PlayerStates;
}

void APortalLobbyGameMode::SetRequiredReadyPlayers(int32 RequiredPlayers)
{
    RequiredReadyPlayers = FMath::Clamp(RequiredPlayers, 1, MaxPlayers);
    BroadcastLobbyUpdate();
}

void APortalLobbyGameMode::SetAutoStartCountdown(float CountdownTime)
{
    AutoStartCountdown = FMath::Max(CountdownTime, 5.0f);
}

void APortalLobbyGameMode::CancelAutoStart()
{
    if (bAutoStartActive) {
        bAutoStartActive = false;
        CurrentCountdownTime = 0.0f;
        GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);
        BroadcastLobbyUpdate();

        UE_LOG(LogTemp, Log, TEXT("Auto start cancelled"));
    }
}

void APortalLobbyGameMode::CheckReadyStatus()
{
    bool bCanStart = CanStartGame();

    if (bCanStart && !bAutoStartActive) {
        StartAutoStartCountdown();
    } else if (!bCanStart && bAutoStartActive) {
        CancelAutoStart();
    }
}

void APortalLobbyGameMode::StartAutoStartCountdown()
{
    bAutoStartActive = true;
    CurrentCountdownTime = AutoStartCountdown;

    GetWorldTimerManager().SetTimer(AutoStartTimerHandle, this, &APortalLobbyGameMode::OnAutoStartTimer, 1.0f, true);

    UE_LOG(LogTemp, Log, TEXT("Auto start countdown began: %.1f seconds"), CurrentCountdownTime);
    BroadcastLobbyUpdate();
}

void APortalLobbyGameMode::HandleAutoStart()
{
    if (CanStartGame()) {
        UE_LOG(LogTemp, Warning, TEXT("Auto starting game"));
        StartGame();
    } else {
        CancelAutoStart();
    }
}

void APortalLobbyGameMode::BroadcastLobbyUpdate()
{
    if (APortalLobbyGameState* PortalGameState = GetGameState<APortalLobbyGameState>()) {
        PortalGameState->UpdateLobbyInfo(
            GetAllPlayerStates(),
            CurrentSelectedMap,
            bAutoStartActive,
            CurrentCountdownTime);
    }
}

void APortalLobbyGameMode::OnAutoStartTimer()
{
    CurrentCountdownTime -= 1.0f;

    if (CurrentCountdownTime <= 0.0f) {
        GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);
        bAutoStartActive = false;
        HandleAutoStart();
    } else {
        BroadcastLobbyUpdate();
    }
}

void APortalLobbyGameMode::OnReadyCheckTimer()
{
    CheckReadyStatus();
}