#include "LobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "MultiplayerLobbyWidget.h"
#include "PortalPlayerState.h"

ALobbyPlayerController::ALobbyPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void ALobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    SetupInputMode();

    // Delay widget creation to ensure game state is ready
    FTimerHandle DelayHandle;
    GetWorldTimerManager().SetTimer(DelayHandle, this, &ALobbyPlayerController::ShowLobbyWidget, 0.5f, false);
}

void ALobbyPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
}

void ALobbyPlayerController::ShowLobbyWidget()
{
    if (!LobbyWidget) {
        CreateLobbyWidget();
    }

    if (LobbyWidget) {
        LobbyWidget->AddToViewport();
        SetupInputMode();
    }
}

void ALobbyPlayerController::HideLobbyWidget()
{
    if (LobbyWidget) {
        LobbyWidget->RemoveFromParent();
    }
}

void ALobbyPlayerController::ServerSetPlayerName_Implementation(const FString& NewPlayerName)
{
    if (APortalPlayerState* PortalPlayerState = GetPlayerState<APortalPlayerState>()) {
        PortalPlayerState->ServerSetPlayerDisplayName(NewPlayerName);
    }
}

void ALobbyPlayerController::SetupInputMode()
{
    FInputModeUIOnly InputMode;
    if (LobbyWidget) {
        InputMode.SetWidgetToFocus(LobbyWidget->TakeWidget());
    }
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

    SetInputMode(InputMode);
    bShowMouseCursor = true;
}

void ALobbyPlayerController::CreateLobbyWidget()
{
    if (LobbyWidgetClass) {
        LobbyWidget = CreateWidget<UMultiplayerLobbyWidget>(this, LobbyWidgetClass);
    } else {
        UE_LOG(LogTemp, Error, TEXT("LobbyWidgetClass not set in ALobbyPlayerController"));
    }
}