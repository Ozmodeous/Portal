#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreMinimal.h"
#include "PortalGameInstance.h"
#include "PortalLobbyGameState.h"
#include "PortalPlayerState.h"
#include "MultiplayerLobbyWidget.generated.h"

UCLASS()
class PORTAL_API UMultiplayerLobbyWidget : public UUserWidget {
    GENERATED_BODY()

public:
    UMultiplayerLobbyWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    // Widget Bindings
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> ReadyButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> StartGameButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> ReturnToMenuButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UComboBoxString> MapSelectionComboBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UVerticalBox> PlayerListVerticalBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> ServerInfoTextBlock;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> CountdownTextBlock;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> ReadyStatusTextBlock;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UProgressBar> CountdownProgressBar;

    // Lobby Management
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdatePlayerList();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdateServerInfo();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdateCountdownDisplay();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdateReadyStatus();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ToggleReady();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void StartGame();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ReturnToMainMenu();

protected:
    // Button Click Events
    UFUNCTION()
    void OnReadyButtonClicked();

    UFUNCTION()
    void OnStartGameButtonClicked();

    UFUNCTION()
    void OnReturnToMenuButtonClicked();

    UFUNCTION()
    void OnMapSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    // Game State Events
    UFUNCTION()
    void OnLobbyUpdated();

    UFUNCTION()
    void OnMapChanged(const FString& NewMapName);

    UFUNCTION()
    void OnCountdownChanged(bool bIsActive, float TimeRemaining);

    // Player State Events
    UFUNCTION()
    void OnPlayerReadyStateChanged(bool bIsReady);

private:
    // References
    UPROPERTY()
    TObjectPtr<UPortalGameInstance> GameInstance;

    UPROPERTY()
    TObjectPtr<APortalLobbyGameState> LobbyGameState;

    UPROPERTY()
    TObjectPtr<APortalPlayerState> LocalPlayerState;

    // UI State
    bool bIsHost;
    bool bIsLocalPlayerReady;
    float LastCountdownTime;

    // Map Selection
    TMap<FString, FString> MapDisplayToPath;

    // Helper Functions
    void InitializeMapSelection();
    void BindEvents();
    void UnbindEvents();
    void RefreshLobbyState();
    UUserWidget* CreatePlayerRowWidget(APortalPlayerState* PlayerState);
    void UpdateButtonStates();
    FString GetMapDisplayName(const FString& MapPath);
    FString GetMapPathFromDisplay(const FString& DisplayName);
};