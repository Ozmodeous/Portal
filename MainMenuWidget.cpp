#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SelectedServerIndex = -1;
    bIsConnecting = false;
    DefaultServerName = TEXT("Portal Defense Server");
    DefaultPlayerName = TEXT("Player");
}

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GameInstance = Cast<UPortalGameInstance>(GetGameInstance());

    InitializeDefaultValues();
    PopulateMapComboBox();
    BindGameInstanceEvents();

    // Bind button events
    if (HostButton) {
        HostButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnHostButtonClicked);
    }

    if (JoinButton) {
        JoinButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinButtonClicked);
    }

    if (RefreshButton) {
        RefreshButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnRefreshButtonClicked);
    }

    if (QuitButton) {
        QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitButtonClicked);
    }

    // Initial server list refresh
    RefreshServerList();
}

void UMainMenuWidget::NativeDestruct()
{
    UnbindGameInstanceEvents();
    Super::NativeDestruct();
}

void UMainMenuWidget::InitializeDefaultValues()
{
    if (ServerNameTextBox) {
        ServerNameTextBox->SetText(FText::FromString(DefaultServerName));
    }

    if (PlayerNameTextBox) {
        PlayerNameTextBox->SetText(FText::FromString(DefaultPlayerName));
    }

    if (LANCheckBox) {
        LANCheckBox->SetIsChecked(false);
    }

    SetStatusText(TEXT("Ready"));
}

void UMainMenuWidget::PopulateMapComboBox()
{
    if (!MapComboBox)
        return;

    MapComboBox->ClearOptions();
    MapComboBox->AddOption(TEXT("Portal Defense"));
    MapComboBox->AddOption(TEXT("Test Arena"));
    MapComboBox->AddOption(TEXT("Coop Defense"));

    MapComboBox->SetSelectedIndex(0);
}

void UMainMenuWidget::BindGameInstanceEvents()
{
    if (GameInstance) {
        GameInstance->OnCreateSessionComplete.AddDynamic(this, &UMainMenuWidget::OnCreateSessionComplete);
        GameInstance->OnFindSessionsComplete.AddDynamic(this, &UMainMenuWidget::OnFindSessionsComplete);
        GameInstance->OnJoinSessionComplete.AddDynamic(this, &UMainMenuWidget::OnJoinSessionComplete);
    }
}

void UMainMenuWidget::UnbindGameInstanceEvents()
{
    if (GameInstance) {
        GameInstance->OnCreateSessionComplete.RemoveDynamic(this, &UMainMenuWidget::OnCreateSessionComplete);
        GameInstance->OnFindSessionsComplete.RemoveDynamic(this, &UMainMenuWidget::OnFindSessionsComplete);
        GameInstance->OnJoinSessionComplete.RemoveDynamic(this, &UMainMenuWidget::OnJoinSessionComplete);
    }
}

void UMainMenuWidget::RefreshServerList()
{
    if (!GameInstance)
        return;

    SetStatusText(TEXT("Searching for servers..."));
    ClearServerList();

    bool bIsLAN = LANCheckBox ? LANCheckBox->IsChecked() : false;
    GameInstance->FindSessions(bIsLAN);
}

void UMainMenuWidget::ClearServerList()
{
    if (ServerListScrollBox) {
        ServerListScrollBox->ClearChildren();
    }
    SelectedServerIndex = -1;
}

void UMainMenuWidget::PopulateServerList(const TArray<FServerInfo>& Servers)
{
    ClearServerList();

    if (!ServerListScrollBox)
        return;

    for (int32 i = 0; i < Servers.Num(); i++) {
        UUserWidget* ServerRowWidget = CreateServerRowWidget(Servers[i], i);
        if (ServerRowWidget) {
            ServerListScrollBox->AddChild(ServerRowWidget);
        }
    }

    if (Servers.Num() == 0) {
        SetStatusText(TEXT("No servers found"));
    } else {
        SetStatusText(FString::Printf(TEXT("Found %d servers"), Servers.Num()));
    }
}

UUserWidget* UMainMenuWidget::CreateServerRowWidget(const FServerInfo& ServerInfo, int32 ServerIndex)
{
    // Create a horizontal box for server row
    UHorizontalBox* ServerRow = NewObject<UHorizontalBox>(this);

    // Server Name
    UTextBlock* ServerNameText = NewObject<UTextBlock>(this);
    ServerNameText->SetText(FText::FromString(ServerInfo.ServerName));
    ServerRow->AddChildToHorizontalBox(ServerNameText);

    // Map Name
    UTextBlock* MapNameText = NewObject<UTextBlock>(this);
    MapNameText->SetText(FText::FromString(ServerInfo.MapName));
    ServerRow->AddChildToHorizontalBox(MapNameText);

    // Player Count
    UTextBlock* PlayerCountText = NewObject<UTextBlock>(this);
    FString PlayerCountString = FString::Printf(TEXT("%d/%d"), ServerInfo.CurrentPlayers, ServerInfo.MaxPlayers);
    PlayerCountText->SetText(FText::FromString(PlayerCountString));
    ServerRow->AddChildToHorizontalBox(PlayerCountText);

    // Ping
    UTextBlock* PingText = NewObject<UTextBlock>(this);
    PingText->SetText(FText::FromString(FString::Printf(TEXT("%d ms"), ServerInfo.Ping)));
    ServerRow->AddChildToHorizontalBox(PingText);

    // Join Button
    UButton* JoinServerButton = NewObject<UButton>(this);
    UTextBlock* JoinButtonText = NewObject<UTextBlock>(this);
    JoinButtonText->SetText(FText::FromString(TEXT("Join")));
    JoinServerButton->AddChild(JoinButtonText);

    // Simple lambda for join button
    JoinServerButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinButtonClicked);

    ServerRow->AddChildToHorizontalBox(JoinServerButton);

    // Create wrapper widget
    UUserWidget* WrapperWidget = NewObject<UUserWidget>(this);
    WrapperWidget->WidgetTree->RootWidget = ServerRow;

    return WrapperWidget;
}

void UMainMenuWidget::HostServer()
{
    if (!GameInstance)
        return;

    FString ServerName = ServerNameTextBox ? ServerNameTextBox->GetText().ToString() : DefaultServerName;
    FString Password = PasswordTextBox ? PasswordTextBox->GetText().ToString() : TEXT("");
    bool bIsLAN = LANCheckBox ? LANCheckBox->IsChecked() : false;

    // Get selected map
    FString SelectedMapName = TEXT("/Game/Maps/PortalDefenseMap");
    if (MapComboBox) {
        FString MapDisplayName = MapComboBox->GetSelectedOption();
        if (MapDisplayName == TEXT("Portal Defense")) {
            SelectedMapName = TEXT("/Game/Maps/PortalDefenseMap");
        } else if (MapDisplayName == TEXT("Test Arena")) {
            SelectedMapName = TEXT("/Game/Maps/TestArena");
        } else if (MapDisplayName == TEXT("Coop Defense")) {
            SelectedMapName = TEXT("/Game/Maps/CoopDefense");
        }
    }

    SetStatusText(TEXT("Creating server..."));
    ShowConnectingStatus();

    GameInstance->CreateSession(ServerName, SelectedMapName, 4, bIsLAN, Password);
}

void UMainMenuWidget::JoinSelectedServer()
{
    if (!GameInstance || SelectedServerIndex < 0)
        return;

    FString Password = PasswordTextBox ? PasswordTextBox->GetText().ToString() : TEXT("");

    SetStatusText(TEXT("Joining server..."));
    ShowConnectingStatus();

    GameInstance->JoinSessionByIndex(SelectedServerIndex, Password);
}

void UMainMenuWidget::QuitGame()
{
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, true);
}

void UMainMenuWidget::SetStatusText(const FString& StatusMessage)
{
    if (StatusTextBlock) {
        StatusTextBlock->SetText(FText::FromString(StatusMessage));
    }
}

void UMainMenuWidget::ShowConnectingStatus()
{
    bIsConnecting = true;
    SetUIEnabled(false);
}

void UMainMenuWidget::HideConnectingStatus()
{
    bIsConnecting = false;
    SetUIEnabled(true);
}

void UMainMenuWidget::SetUIEnabled(bool bEnabled)
{
    if (HostButton)
        HostButton->SetIsEnabled(bEnabled);
    if (JoinButton)
        JoinButton->SetIsEnabled(bEnabled);
    if (RefreshButton)
        RefreshButton->SetIsEnabled(bEnabled);
    if (ServerNameTextBox)
        ServerNameTextBox->SetIsEnabled(bEnabled);
    if (PlayerNameTextBox)
        PlayerNameTextBox->SetIsEnabled(bEnabled);
    if (PasswordTextBox)
        PasswordTextBox->SetIsEnabled(bEnabled);
    if (MapComboBox)
        MapComboBox->SetIsEnabled(bEnabled);
    if (LANCheckBox)
        LANCheckBox->SetIsEnabled(bEnabled);
}

void UMainMenuWidget::OnHostButtonClicked()
{
    HostServer();
}

void UMainMenuWidget::OnJoinButtonClicked()
{
    JoinSelectedServer();
}

void UMainMenuWidget::OnRefreshButtonClicked()
{
    RefreshServerList();
}

void UMainMenuWidget::OnQuitButtonClicked()
{
    QuitGame();
}

void UMainMenuWidget::OnServerRowSelected(int32 ServerIndex)
{
    SelectedServerIndex = ServerIndex;
    JoinSelectedServer();
}

void UMainMenuWidget::OnCreateSessionComplete(bool bWasSuccessful)
{
    HideConnectingStatus();

    if (bWasSuccessful) {
        SetStatusText(TEXT("Server created successfully!"));
    } else {
        SetStatusText(TEXT("Failed to create server"));
    }
}

void UMainMenuWidget::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (bWasSuccessful && GameInstance) {
        TArray<FServerInfo> FoundServers = GameInstance->GetFoundServers();
        PopulateServerList(FoundServers);
    } else {
        SetStatusText(TEXT("Failed to find servers"));
        ClearServerList();
    }
}

void UMainMenuWidget::OnJoinSessionComplete(bool bWasSuccessful)
{
    HideConnectingStatus();

    if (!bWasSuccessful) {
        SetStatusText(TEXT("Failed to join server"));
    }
}