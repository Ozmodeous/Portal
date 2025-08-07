#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/TimerHandle.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "PortalGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPortalOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPortalOnFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPortalOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPortalOnDestroySessionComplete, bool, bWasSuccessful);

USTRUCT(BlueprintType)
struct FServerInfo {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ServerName;

    UPROPERTY(BlueprintReadOnly)
    FString MapName;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers;

    UPROPERTY(BlueprintReadOnly)
    int32 Ping;

    UPROPERTY(BlueprintReadOnly)
    bool bIsPasswordProtected;

    int32 SearchResultIndex;

    FServerInfo()
    {
        ServerName = TEXT("");
        MapName = TEXT("");
        CurrentPlayers = 0;
        MaxPlayers = 0;
        Ping = 0;
        bIsPasswordProtected = false;
        SearchResultIndex = -1;
    }
};

UCLASS()
class PORTAL_API UPortalGameInstance : public UGameInstance {
    GENERATED_BODY()

public:
    UPortalGameInstance();

protected:
    virtual void Init() override;
    virtual void Shutdown() override;

public:
    // Session Management
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void CreateSession(const FString& ServerName, const FString& MapName, int32 MaxPlayers = 4, bool bIsLAN = false, const FString& Password = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void FindSessions(bool bIsLAN = false);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinSessionByIndex(int32 SearchResultIndex, const FString& Password = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void DestroySession();

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void StartSession();

    // Server Browser
    UFUNCTION(BlueprintPure, Category = "Multiplayer")
    TArray<FServerInfo> GetFoundServers() const { return FoundServers; }

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void RefreshServerList();

    // Level Management
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void OpenMainMenu();

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void OpenLobby(const FString& LobbyMap = TEXT("/Game/Maps/LobbyMap"));

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void StartGame(const FString& GameMap);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void ReturnToMainMenu();

    // Session Info
    UFUNCTION(BlueprintPure, Category = "Multiplayer")
    bool IsInSession() const;

    UFUNCTION(BlueprintPure, Category = "Multiplayer")
    bool IsSessionHost() const;

    UFUNCTION(BlueprintPure, Category = "Multiplayer")
    FString GetCurrentSessionName() const { return CurrentSessionName; }

    UFUNCTION(BlueprintPure, Category = "Multiplayer")
    FString GetCurrentMapName() const { return CurrentMapName; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
    FPortalOnCreateSessionComplete OnCreateSessionComplete;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
    FPortalOnFindSessionsComplete OnFindSessionsComplete;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
    FPortalOnJoinSessionComplete OnJoinSessionComplete;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
    FPortalOnDestroySessionComplete OnDestroySessionComplete;

protected:
    // Online Session Interface
    IOnlineSessionPtr SessionInterface;

    // Session Search
    TSharedPtr<FOnlineSessionSearch> SessionSearch;

    // Found Servers
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    TArray<FServerInfo> FoundServers;

    // Current Session Info
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    FString CurrentSessionName;

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    FString CurrentMapName;

    // Session Settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multiplayer")
    int32 DefaultMaxPlayers = 4;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multiplayer")
    FString DefaultGameMap = TEXT("/Game/Maps/PortalDefenseMap");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multiplayer")
    FString LobbyMapPath = TEXT("/Game/Maps/LobbyMap");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multiplayer")
    FString MainMenuMapPath = TEXT("/Game/Maps/MainMenuMap");

private:
    // Session Delegates
    FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;

    // Delegate Handles
    FDelegateHandle CreateSessionCompleteDelegateHandle;
    FDelegateHandle StartSessionCompleteDelegateHandle;
    FDelegateHandle FindSessionsCompleteDelegateHandle;
    FDelegateHandle JoinSessionCompleteDelegateHandle;
    FDelegateHandle DestroySessionCompleteDelegateHandle;

    // Timer for refreshing server list
    FTimerHandle RefreshServerListTimer;

    // Session Callback Functions
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    // Helper Functions
    void InitializeSessionInterface();
    void CleanupSessionDelegates();
    FServerInfo ConvertSearchResultToServerInfo(const FOnlineSessionSearchResult& SearchResult, int32 Index);
};