#include "MultiplayerLobbyWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PortalLobbyGameMode.h"

UMultiplayerLobbyWidget::UMultiplayerLobbyWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsHost = false;
    bIsLocalPlayerReady = false;
    LastCountdownTime = 0.0f;
}

void UMultiplayerLobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GameInstance = Cast<UPortalGameInstance>(GetGameInstance());
    bIsHost = GameInstance ? GameInstance->IsSessionHost() : false;

    InitializeMapSelection();
    BindEvents();
    RefreshLobbyState();

    // Bind button events
    if (ReadyButton) {
        ReadyButton->OnClicked.AddDynamic(this, &UMultiplayerLobbyWidget::OnReadyButtonClicked);
    }

    if (StartGameButton) {
        StartGameButton->OnClicked.AddDynamic(this, &UMultiplayerLobbyWidget::OnStartGameButtonClicked);
    }

    if (ReturnToMenuButton) {
        ReturnToMenuButton->OnClicked.AddDynamic(this, &UMultiplayerLobbyWidget::OnReturnToMenuButtonClicked);
    }

    if (MapSelectionComboBox) {
        MapSelectionComboBox->OnSelectionChanged.AddDynamic(this, &UMultiplayerLobbyWidget::OnMapSelectionChanged);
    }

    UpdateButtonStates();
}

void UMultiplayerLobbyWidget::NativeDestruct()
{
    UnbindEvents();
    Super::NativeDestruct();
}

void UMultiplayerLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Refresh lobby state periodically
    RefreshLobbyState();
}

void UMultiplayerLobbyWidget::InitializeMapSelection()
{
    MapDisplayToPath.Empty();
    MapDisplayToPath.Add(TEXT("Portal Defense"), TEXT("/Game/Maps/PortalDefenseMap"));
    MapDisplayToPath.Add(TEXT("Test Arena"), TEXT("/Game/Maps/TestArena"));
    MapDisplayToPath.Add(TEXT("Coop Defense"), TEXT("/Game/Maps/CoopDefense"));

    if (MapSelectionComboBox) {
        MapSelectionComboBox->ClearOptions();
        for (const auto& MapPair : MapDisplayToPath) {
            MapSelectionComboBox->AddOption(MapPair.Key);
        }
        MapSelectionComboBox->SetSelectedIndex(0);
    }
}

void UMultiplayerLobbyWidget::BindEvents()
{
    if (UWorld* World = GetWorld()) {
        LobbyGameState = World->GetGameState<APortalLobbyGameState>();
        if (LobbyGameState) {
            LobbyGameState->OnLobbyUpdated.AddDynamic(this, &UMultiplayerLobbyWidget::OnLobbyUpdated);
            LobbyGameState->OnMapChanged.AddDynamic(this, &UMultiplayerLobbyWidget::OnMapChanged);
            LobbyGameState->OnCountdownChanged.AddDynamic(this, &UMultiplayerLobbyWidget::OnCountdownChanged);
        }

        if (APlayerController* PC = GetOwningPlayer()) {
            LocalPlayerState = PC->GetPlayerState<APortalPlayerState>();
            if (LocalPlayerState) {
                LocalPlayerState->OnPlayerReadyStateChanged.AddDynamic(this, &UMultiplayerLobbyWidget::OnPlayerReadyStateChanged);
            }
        }
    }
}

void UMultiplayerLobbyWidget::UnbindEvents()
{
    if (LobbyGameState) {
        LobbyGameState->OnLobbyUpdated.RemoveDynamic(this, &UMultiplayerLobbyWidget::OnLobbyUpdated);
        LobbyGameState->OnMapChanged.RemoveDynamic(this, &UMultiplayerLobbyWidget::OnMapChanged);
        LobbyGameState->OnCountdownChanged.RemoveDynamic(this, &UMultiplayerLobbyWidget::OnCountdownChanged);
    }

    if (LocalPlayerState) {
        LocalPlayerState->OnPlayerReadyStateChanged.RemoveDynamic(this, &UMultiplayerLobbyWidget::OnPlayerReadyStateChanged);
    }
}

void UMultiplayerLobbyWidget::RefreshLobbyState()
{
    if (!LobbyGameState) {
        if (UWorld* World = GetWorld()) {
            LobbyGameState = World->GetGameState<APortalLobbyGameState>();
        }
    }

    if (!LocalPlayerState) {
        if (APlayerController* PC = GetOwningPlayer()) {
            LocalPlayerState = PC->GetPlayerState<APortalPlayerState>();
        }
    }
}

void UMultiplayerLobbyWidget::UpdatePlayerList()
{
    if (!PlayerListVerticalBox || !LobbyGameState)
        return;

    PlayerListVerticalBox->ClearChildren();

    TArray<APortalPlayerState*> Players = LobbyGameState->GetLobbyPlayers();
    for (APortalPlayerState* Player : Players) {
        if (Player) {
            UUserWidget* PlayerRowWidget = CreatePlayerRowWidget(Player);
            if (PlayerRowWidget) {
                PlayerListVerticalBox->AddChild(PlayerRowWidget);
            }
        }
    }
}

UUserWidget* UMultiplayerLobbyWidget::CreatePlayerRowWidget(APortalPlayerState* PlayerState)
{
    if (!PlayerState)
        return nullptr;

    // Create horizontal box for player row
    UHorizontalBox* PlayerRow = NewObject<UHorizontalBox>(this);

    // Player Name
    UTextBlock* PlayerNameText = NewObject<UTextBlock>(this);
    PlayerNameText->SetText(FText::FromString(PlayerState->GetPlayerDisplayName()));
    PlayerRow->AddChild(PlayerNameText);

    // Ready Status
    UTextBlock* ReadyStatusText = NewObject<UTextBlock>(this);
    FString ReadyText = PlayerState->IsReady() ? TEXT("READY") : TEXT("NOT READY");
    FLinearColor ReadyColor = PlayerState->IsReady() ? FLinearColor::Green : FLinearColor::Red;
    ReadyStatusText->SetText(FText::FromString(ReadyText));
    ReadyStatusText->SetColorAndOpacity(FSlateColor(ReadyColor));
    PlayerRow->AddChild(ReadyStatusText);

    // Host Indicator
    if (bIsHost && PlayerState == LocalPlayerState) {
        UTextBlock* HostText = NewObject<UTextBlock>(this);
        HostText->SetText(FText::FromString(TEXT("(HOST)")));
        HostText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
        PlayerRow->AddChild(HostText);
    }

    // Create wrapper widget using UE 5.5 pattern
    UUserWidget* WrapperWidget = NewObject<UUserWidget>(this);
    WrapperWidget->WidgetTree->RootWidget = PlayerRow;

    return WrapperWidget;
}

void UMultiplayerLobbyWidget::UpdateServerInfo()
{
    if (!ServerInfoTextBlock || !GameInstance)
        return;

    FString ServerName = GameInstance->GetCurrentSessionName();
    FString MapName = GetMapDisplayName(GameInstance->GetCurrentMapName());

    FString ServerInfo = FString::Printf(TEXT("Server: %s | Map: %s"), *ServerName, *MapName);
    ServerInfoTextBlock->SetText(FText::FromString(ServerInfo));
}

void UMultiplayerLobbyWidget::UpdateCountdownDisplay()
{
    if (!LobbyGameState)
        return;

    bool bCountdownActive = LobbyGameState->IsCountdownActive();
    float CountdownTime = LobbyGameState->GetCountdownTime();

    if (CountdownTextBlock) {
        if (bCountdownActive) {
            FString CountdownText = FString::Printf(TEXT("Game starting in: %.0f"), CountdownTime);
            CountdownTextBlock->SetText(FText::FromString(CountdownText));
            CountdownTextBlock->SetVisibility(ESlateVisibility::Visible);
        } else {
            CountdownTextBlock->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (CountdownProgressBar) {
        if (bCountdownActive) {
            float MaxCountdownTime = 10.0f; // Should match game mode setting
            float Progress = 1.0f - (CountdownTime / MaxCountdownTime);
            CountdownProgressBar->SetPercent(Progress);
            CountdownProgressBar->SetVisibility(ESlateVisibility::Visible);
        } else {
            CountdownProgressBar->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMultiplayerLobbyWidget::UpdateReadyStatus()
{
    if (!ReadyStatusTextBlock || !LobbyGameState)
        return;

    int32 ReadyCount = LobbyGameState->GetReadyPlayerCount();
    int32 TotalCount = LobbyGameState->GetTotalPlayerCount();

    FString ReadyStatusText = FString::Printf(TEXT("Ready: %d/%d"), ReadyCount, TotalCount);
    ReadyStatusTextBlock->SetText(FText::FromString(ReadyStatusText));
}

void UMultiplayerLobbyWidget::UpdateButtonStates()
{
    if (ReadyButton && LocalPlayerState) {
        FString ReadyButtonText = LocalPlayerState->IsReady() ? TEXT("UNREADY") : TEXT("READY");
        if (UTextBlock* ReadyButtonTextBlock = Cast<UTextBlock>(ReadyButton->GetChildAt(0))) {
            ReadyButtonTextBlock->SetText(FText::FromString(ReadyButtonText));
        }
    }

    if (StartGameButton) {
        bool bCanStartGame = false;
        if (UWorld* World = GetWorld()) {
            if (APortalLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<APortalLobbyGameMode>()) {
                bCanStartGame = bIsHost && LobbyGameMode->CanStartGame();
            }
        }
        StartGameButton->SetIsEnabled(bCanStartGame);
        StartGameButton->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (MapSelectionComboBox) {
        MapSelectionComboBox->SetIsEnabled(bIsHost);
    }
}

void UMultiplayerLobbyWidget::ToggleReady()
{
    if (LocalPlayerState) {
        bool bNewReadyState = !LocalPlayerState->IsReady();
        LocalPlayerState->ServerSetReady(bNewReadyState);
    }
}

void UMultiplayerLobbyWidget::StartGame()
{
    if (bIsHost) {
        if (UWorld* World = GetWorld()) {
            if (APortalLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<APortalLobbyGameMode>()) {
                LobbyGameMode->StartGame();
            }
        }
    }
}

void UMultiplayerLobbyWidget::ReturnToMainMenu()
{
    if (GameInstance) {
        GameInstance->ReturnToMainMenu();
    }
}

FString UMultiplayerLobbyWidget::GetMapDisplayName(const FString& MapPath)
{
    for (const auto& MapPair : MapDisplayToPath) {
        if (MapPair.Value == MapPath) {
            return MapPair.Key;
        }
    }
    return TEXT("Unknown Map");
}

FString UMultiplayerLobbyWidget::GetMapPathFromDisplay(const FString& DisplayName)
{
    if (const FString* MapPath = MapDisplayToPath.Find(DisplayName)) {
        return *MapPath;
    }
    return TEXT("/Game/Maps/PortalDefenseMap");
}

void UMultiplayerLobbyWidget::OnReadyButtonClicked()
{
    ToggleReady();
}

void UMultiplayerLobbyWidget::OnStartGameButtonClicked()
{
    StartGame();
}

void UMultiplayerLobbyWidget::OnReturnToMenuButtonClicked()
{
    ReturnToMainMenu();
}

void UMultiplayerLobbyWidget::OnMapSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (bIsHost && SelectionType != ESelectInfo::OnNavigation) {
        FString MapPath = GetMapPathFromDisplay(SelectedItem);
        if (UWorld* World = GetWorld()) {
            if (APortalLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<APortalLobbyGameMode>()) {
                LobbyGameMode->ChangeMap(MapPath);
            }
        }
    }
}

void UMultiplayerLobbyWidget::OnLobbyUpdated()
{
    UpdatePlayerList();
    UpdateServerInfo();
    UpdateCountdownDisplay();
    UpdateReadyStatus();
    UpdateButtonStates();
}

void UMultiplayerLobbyWidget::OnMapChanged(const FString& NewMapName)
{
    if (MapSelectionComboBox) {
        FString MapDisplayName = GetMapDisplayName(NewMapName);
        MapSelectionComboBox->SetSelectedOption(MapDisplayName);
    }
    UpdateServerInfo();
}

void UMultiplayerLobbyWidget::OnCountdownChanged(bool bIsActive, float TimeRemaining)
{
    UpdateCountdownDisplay();
}

void UMultiplayerLobbyWidget::OnPlayerReadyStateChanged(bool bIsReady)
{
    bIsLocalPlayerReady = bIsReady;
    UpdateButtonStates();
}