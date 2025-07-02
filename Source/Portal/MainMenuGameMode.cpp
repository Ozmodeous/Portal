#include "MainMenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "MainMenuPlayerController.h"
#include "MainMenuWidget.h"

AMainMenuGameMode::AMainMenuGameMode()
{
    PlayerControllerClass = AMainMenuPlayerController::StaticClass();

    // Don't spawn default pawns in main menu
    DefaultPawnClass = nullptr;

    // Set this to false to prevent automatic spawning
    bStartPlayersAsSpectators = true;
}

void AMainMenuGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Show main menu UI after a brief delay
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AMainMenuGameMode::ShowMainMenuWidget, 0.1f, false);
}

void AMainMenuGameMode::RestartPlayer(AController* NewPlayer)
{
    // Don't spawn pawns in main menu
}

void AMainMenuGameMode::ShowMainMenuWidget()
{
    // Get the first player controller
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
        if (AMainMenuPlayerController* MenuPC = Cast<AMainMenuPlayerController>(PC)) {
            MenuPC->ShowMainMenu();
        }
    }
}