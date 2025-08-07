#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class UMultiplayerLobbyWidget;

UCLASS()
class PORTAL_API ALobbyPlayerController : public APlayerController {
    GENERATED_BODY()

public:
    ALobbyPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

public:
    // Lobby UI Management
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ShowLobbyWidget();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void HideLobbyWidget();

    UFUNCTION(BlueprintPure, Category = "Lobby")
    UMultiplayerLobbyWidget* GetLobbyWidget() const { return LobbyWidget; }

    // Player Management
    UFUNCTION(BlueprintCallable, Category = "Player", Server, Reliable)
    void ServerSetPlayerName(const FString& NewPlayerName);

protected:
    // Widget Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UMultiplayerLobbyWidget> LobbyWidgetClass;

    // Current Widget
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    TObjectPtr<UMultiplayerLobbyWidget> LobbyWidget;

private:
    void SetupInputMode();
    void CreateLobbyWidget();
    void ServerSetPlayerName_Implementation(const FString& NewPlayerName);
};