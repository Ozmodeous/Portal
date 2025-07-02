#include "PortalGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

UPortalGameInstance::UPortalGameInstance()
{
    CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UPortalGameInstance::HandleCreateSessionComplete);
    StartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &UPortalGameInstance::HandleStartSessionComplete);
    FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UPortalGameInstance::HandleFindSessionsComplete);
    JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UPortalGameInstance::HandleJoinSessionComplete);
    DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UPortalGameInstance::HandleDestroySessionComplete);
}

void UPortalGameInstance::Init()
{
    Super::Init();
    InitializeSessionInterface();
}

void UPortalGameInstance::Shutdown()
{
    CleanupSessionDelegates();
    Super::Shutdown();
}

void UPortalGameInstance::InitializeSessionInterface()
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    if (OnlineSubsystem) {
        SessionInterface = OnlineSubsystem->GetSessionInterface();
        if (SessionInterface.IsValid()) {
            UE_LOG(LogTemp, Warning, TEXT("Online Subsystem: %s"), *OnlineSubsystem->GetSubsystemName().ToString());
        } else {
            UE_LOG(LogTemp, Error, TEXT("Failed to get Session Interface"));
        }
    } else {
        UE_LOG(LogTemp, Error, TEXT("No Online Subsystem found"));
    }
}

void UPortalGameInstance::CleanupSessionDelegates()
{
    if (SessionInterface.IsValid()) {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
    }
}

void UPortalGameInstance::CreateSession(const FString& ServerName, const FString& MapName, int32 MaxPlayers, bool bIsLAN, const FString& Password)
{
    if (!SessionInterface.IsValid()) {
        OnCreateSessionComplete.Broadcast(false);
        return;
    }

    // Destroy existing session if one exists
    if (SessionInterface->GetNamedSession(NAME_GameSession)) {
        SessionInterface->DestroySession(NAME_GameSession);
    }

    // Store session info
    CurrentSessionName = ServerName;
    CurrentMapName = MapName;

    // Create session settings
    TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());

    SessionSettings->bIsLANMatch = bIsLAN;
    SessionSettings->NumPublicConnections = MaxPlayers;
    SessionSettings->NumPrivateConnections = 0;
    SessionSettings->bAllowInvites = true;
    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bUsesPresence = !bIsLAN;
    SessionSettings->bUseLobbiesIfAvailable = true;
    SessionSettings->bAllowJoinViaPresence = !bIsLAN;
    SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;

    // Set custom settings
    SessionSettings->Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    SessionSettings->Set(FName("MAP_NAME"), MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    if (!Password.IsEmpty()) {
        SessionSettings->Set(FName("PASSWORD"), Password, EOnlineDataAdvertisementType::ViaOnlineService);
        SessionSettings->Set(FName("HAS_PASSWORD"), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    } else {
        SessionSettings->Set(FName("HAS_PASSWORD"), false, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    }

    // Bind delegate and create session
    CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

    const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings)) {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        OnCreateSessionComplete.Broadcast(false);
    }
}

void UPortalGameInstance::FindSessions(bool bIsLAN)
{
    if (!SessionInterface.IsValid()) {
        OnFindSessionsComplete.Broadcast(false);
        return;
    }

    // Clear previous search results
    FoundServers.Empty();

    // Create session search
    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = bIsLAN;
    SessionSearch->MaxSearchResults = 50;
    SessionSearch->PingBucketSize = 50;

    // Set query settings
    if (bIsLAN) {
        SessionSearch->QuerySettings.Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineComparisonOp::Equals);
    } else {
        SessionSearch->QuerySettings.Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineComparisonOp::Equals);
    }

    // Bind delegate and start search
    FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

    const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef())) {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
        OnFindSessionsComplete.Broadcast(false);
    }
}

void UPortalGameInstance::JoinSessionByIndex(int32 SearchResultIndex, const FString& Password)
{
    if (!SessionInterface.IsValid() || !SessionSearch.IsValid()) {
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    if (!SessionSearch->SearchResults.IsValidIndex(SearchResultIndex)) {
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[SearchResultIndex];

    // Check password if required
    bool bHasPassword = false;
    if (SearchResult.Session.SessionSettings.Get(FName("HAS_PASSWORD"), bHasPassword) && bHasPassword) {
        FString ServerPassword;
        if (SearchResult.Session.SessionSettings.Get(FName("PASSWORD"), ServerPassword)) {
            if (ServerPassword != Password) {
                UE_LOG(LogTemp, Warning, TEXT("Incorrect password for server"));
                OnJoinSessionComplete.Broadcast(false);
                return;
            }
        }
    }

    // Store session info
    FString ServerName, MapName;
    SearchResult.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);
    SearchResult.Session.SessionSettings.Get(FName("MAP_NAME"), MapName);
    CurrentSessionName = ServerName;
    CurrentMapName = MapName;

    // Bind delegate and join session
    JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

    const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult)) {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        OnJoinSessionComplete.Broadcast(false);
    }
}

void UPortalGameInstance::DestroySession()
{
    if (!SessionInterface.IsValid()) {
        OnDestroySessionComplete.Broadcast(false);
        return;
    }

    DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

    if (!SessionInterface->DestroySession(NAME_GameSession)) {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        OnDestroySessionComplete.Broadcast(false);
    }
}

void UPortalGameInstance::StartSession()
{
    if (!SessionInterface.IsValid()) {
        return;
    }

    StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

    if (!SessionInterface->StartSession(NAME_GameSession)) {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
    }
}

void UPortalGameInstance::RefreshServerList()
{
    FindSessions(false); // Default to internet search
}

void UPortalGameInstance::OpenMainMenu()
{
    UGameplayStatics::OpenLevel(this, FName(*MainMenuMapPath));
}

void UPortalGameInstance::OpenLobby(const FString& LobbyMap)
{
    UGameplayStatics::OpenLevel(this, FName(*LobbyMap));
}

void UPortalGameInstance::StartGame(const FString& GameMap)
{
    if (IsSessionHost()) {
        GetWorld()->ServerTravel(GameMap);
    }
}

void UPortalGameInstance::ReturnToMainMenu()
{
    // Clean up session first
    if (IsInSession()) {
        DestroySession();
    }

    // Return to main menu
    UGameplayStatics::OpenLevel(this, FName(*MainMenuMapPath));
}

bool UPortalGameInstance::IsInSession() const
{
    if (!SessionInterface.IsValid()) {
        return false;
    }

    return SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

bool UPortalGameInstance::IsSessionHost() const
{
    if (!SessionInterface.IsValid()) {
        return false;
    }

    FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
    if (Session) {
        return Session->bHosting;
    }

    return false;
}

void UPortalGameInstance::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

    if (bWasSuccessful) {
        UE_LOG(LogTemp, Warning, TEXT("Session created successfully"));

        // Start the session and open lobby
        StartSession();
        OpenLobby(LobbyMapPath);
    } else {
        UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
    }

    OnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UPortalGameInstance::HandleStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
    SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);

    if (bWasSuccessful) {
        UE_LOG(LogTemp, Warning, TEXT("Session started successfully"));
    } else {
        UE_LOG(LogTemp, Error, TEXT("Failed to start session"));
    }
}

void UPortalGameInstance::HandleFindSessionsComplete(bool bWasSuccessful)
{
    SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

    if (bWasSuccessful && SessionSearch.IsValid()) {
        UE_LOG(LogTemp, Warning, TEXT("Found %d sessions"), SessionSearch->SearchResults.Num());

        // Convert search results to server info
        FoundServers.Empty();
        for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++) {
            FServerInfo ServerInfo = ConvertSearchResultToServerInfo(SessionSearch->SearchResults[i], i);
            FoundServers.Add(ServerInfo);
        }
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find sessions"));
        FoundServers.Empty();
    }

    OnFindSessionsComplete.Broadcast(bWasSuccessful);
}

void UPortalGameInstance::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

    bool bWasSuccessful = (Result == EOnJoinSessionCompleteResult::Success);

    if (bWasSuccessful) {
        UE_LOG(LogTemp, Warning, TEXT("Successfully joined session"));

        // Get travel URL and join
        FString TravelURL;
        if (SessionInterface->GetResolvedConnectString(SessionName, TravelURL)) {
            APlayerController* PlayerController = GetFirstLocalPlayerController();
            if (PlayerController) {
                PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
            }
        }
    } else {
        UE_LOG(LogTemp, Error, TEXT("Failed to join session. Result: %d"), (int32)Result);
    }

    OnJoinSessionComplete.Broadcast(bWasSuccessful);
}

void UPortalGameInstance::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

    if (bWasSuccessful) {
        UE_LOG(LogTemp, Warning, TEXT("Session destroyed successfully"));
    } else {
        UE_LOG(LogTemp, Error, TEXT("Failed to destroy session"));
    }

    OnDestroySessionComplete.Broadcast(bWasSuccessful);
}

FServerInfo UPortalGameInstance::ConvertSearchResultToServerInfo(const FOnlineSessionSearchResult& SearchResult, int32 Index)
{
    FServerInfo ServerInfo;

    ServerInfo.SearchResultIndex = Index;
    ServerInfo.CurrentPlayers = SearchResult.Session.SessionSettings.NumPublicConnections - SearchResult.Session.NumOpenPublicConnections;
    ServerInfo.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
    ServerInfo.Ping = SearchResult.PingInMs;

    // Get custom settings
    SearchResult.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerInfo.ServerName);
    SearchResult.Session.SessionSettings.Get(FName("MAP_NAME"), ServerInfo.MapName);
    SearchResult.Session.SessionSettings.Get(FName("HAS_PASSWORD"), ServerInfo.bIsPasswordProtected);

    // Default values if custom settings not found
    if (ServerInfo.ServerName.IsEmpty()) {
        ServerInfo.ServerName = TEXT("Unknown Server");
    }

    if (ServerInfo.MapName.IsEmpty()) {
        ServerInfo.MapName = TEXT("Unknown Map");
    }

    return ServerInfo;
}