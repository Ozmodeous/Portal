#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "CoreMinimal.h"
#include "PortalGameInstance.h"
#include "MainMenuWidget.generated.h"

UCLASS()
class PORTAL_API UMainMenuWidget : public UUserWidget {
    GENERATED_BODY()

public:
    UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

public:
    // Widget Bindings
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> HostButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> JoinButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> RefreshButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ServerNameTextBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UEditableTextBox> PlayerNameTextBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UEditableTextBox> PasswordTextBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UComboBoxString> MapComboBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCheckBox> LANCheckBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UScrollBox> ServerListScrollBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusTextBlock;

    // Server List Management
    UFUNCTION(BlueprintCallable, Category = "Server List")
    void RefreshServerList();

    UFUNCTION(BlueprintCallable, Category = "Server List")
    void ClearServerList();

    UFUNCTION(BlueprintCallable, Category = "Server List")
    void PopulateServerList(const TArray<FServerInfo>& Servers);

    // Menu Actions
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void HostServer();

    UFUNCTION(BlueprintCallable, Category = "Menu")
    void JoinSelectedServer();

    UFUNCTION(BlueprintCallable, Category = "Menu")
    void QuitGame();

    // Status Updates
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetStatusText(const FString& StatusMessage);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowConnectingStatus();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideConnectingStatus();

protected:
    // Button Click Events
    UFUNCTION()
    void OnHostButtonClicked();

    UFUNCTION()
    void OnJoinButtonClicked();

    UFUNCTION()
    void OnRefreshButtonClicked();

    UFUNCTION()
    void OnQuitButtonClicked();

    // Server Row Selection
    UFUNCTION()
    void OnServerRowSelected(int32 ServerIndex);

    // Game Instance Events
    UFUNCTION()
    void OnCreateSessionComplete(bool bWasSuccessful);

    UFUNCTION()
    void OnFindSessionsComplete(bool bWasSuccessful);

    UFUNCTION()
    void OnJoinSessionComplete(bool bWasSuccessful);

private:
    // Game Instance Reference
    UPROPERTY()
    TObjectPtr<UPortalGameInstance> GameInstance;

    // Selected Server
    int32 SelectedServerIndex;

    // Default Values
    FString DefaultServerName;
    FString DefaultPlayerName;

    // UI State
    bool bIsConnecting;

    // Helper Functions
    void InitializeDefaultValues();
    void BindGameInstanceEvents();
    void UnbindGameInstanceEvents();
    void SetUIEnabled(bool bEnabled);
    void PopulateMapComboBox();

    // Server List Row Widget Creation
    class UUserWidget* CreateServerRowWidget(const FServerInfo& ServerInfo, int32 ServerIndex);
};