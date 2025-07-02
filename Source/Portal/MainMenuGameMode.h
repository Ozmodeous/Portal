#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class PORTAL_API AMainMenuGameMode : public AGameModeBase {
    GENERATED_BODY()

public:
    AMainMenuGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void RestartPlayer(AController* NewPlayer) override;

public:
    // Main Menu Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
    TSubclassOf<class UMainMenuWidget> MainMenuWidgetClass;

protected:
    // Helper Functions
    void ShowMainMenuWidget();
};