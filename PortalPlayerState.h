#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "PortalPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerReadyStateChanged, bool, bIsReady);

UCLASS()
class PORTAL_API APortalPlayerState : public APlayerState {
    GENERATED_BODY()

public:
    APortalPlayerState();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Ready System
    UFUNCTION(BlueprintCallable, Category = "Lobby", Server, Reliable)
    void ServerSetReady(bool bNewReady);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsReady() const { return bIsReady; }

    // Player Info
    UFUNCTION(BlueprintCallable, Category = "Player", Server, Reliable)
    void ServerSetPlayerDisplayName(const FString& NewDisplayName);

    UFUNCTION(BlueprintPure, Category = "Player")
    FString GetPlayerDisplayName() const { return PlayerDisplayName; }

    // Character Selection (for future expansion)
    UFUNCTION(BlueprintCallable, Category = "Character", Server, Reliable)
    void ServerSetSelectedCharacterClass(int32 CharacterClassIndex);

    UFUNCTION(BlueprintPure, Category = "Character")
    int32 GetSelectedCharacterClass() const { return SelectedCharacterClass; }

    // Team Assignment (for future expansion)
    UFUNCTION(BlueprintCallable, Category = "Team", Server, Reliable)
    void ServerSetTeamID(int32 NewTeamID);

    UFUNCTION(BlueprintPure, Category = "Team")
    int32 GetTeamID() const { return TeamID; }

    // Lobby Status
    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsInLobby() const { return bIsInLobby; }

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetInLobby(bool bInLobby) { bIsInLobby = bInLobby; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlayerReadyStateChanged OnPlayerReadyStateChanged;

protected:
    // Replicated Properties
    UPROPERTY(ReplicatedUsing = OnRep_IsReady, BlueprintReadOnly, Category = "Lobby")
    bool bIsReady;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    FString PlayerDisplayName;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character")
    int32 SelectedCharacterClass;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
    int32 TeamID;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby")
    bool bIsInLobby;

    // Replication Functions
    UFUNCTION()
    void OnRep_IsReady();

private:
    void ServerSetReady_Implementation(bool bNewReady);
    void ServerSetPlayerDisplayName_Implementation(const FString& NewDisplayName);
    void ServerSetSelectedCharacterClass_Implementation(int32 CharacterClassIndex);
    void ServerSetTeamID_Implementation(int32 NewTeamID);
};