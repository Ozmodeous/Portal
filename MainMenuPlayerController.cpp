#include "MainMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "MainMenuWidget.h"

AMainMenuPlayerController::AMainMenuPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AMainMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    SetupInputMode();
}

void AMainMenuPlayerController::ShowMainMenu()
{
    if (!MainMenuWidget) {
        CreateMainMenuWidget();
    }

    if (MainMenuWidget) {
        MainMenuWidget->AddToViewport();
        SetupInputMode();
    }
}

void AMainMenuPlayerController::HideMainMenu()
{
    if (MainMenuWidget) {
        MainMenuWidget->RemoveFromParent();
    }
}

void AMainMenuPlayerController::SetupInputMode()
{
    FInputModeUIOnly InputMode;
    if (MainMenuWidget) {
        InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
    }
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

    SetInputMode(InputMode);
    bShowMouseCursor = true;
}

void AMainMenuPlayerController::CreateMainMenuWidget()
{
    if (MainMenuWidgetClass) {
        MainMenuWidget = CreateWidget<UMainMenuWidget>(this, MainMenuWidgetClass);
    } else {
        UE_LOG(LogTemp, Error, TEXT("MainMenuWidgetClass not set in MainMenuPlayerController"));
    }
}