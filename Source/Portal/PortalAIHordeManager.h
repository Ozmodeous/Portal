// PortalAIHordeManager.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "PortalAIHordeComponent.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PortalAIHordeManager.generated.h"

// Forward declarations
class APortalACFAIController;

/**
 * Horde group information
 */
USTRUCT(BlueprintType)
struct FHordeGroup {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 HordeID = -1;

    UPROPERTY(BlueprintReadOnly)
    TArray<UPortalAIHordeComponent*> Members;

    UPROPERTY(BlueprintReadOnly)
    UPortalAIHordeComponent* Leader = nullptr;

    UPROPERTY(BlueprintReadOnly)
    EHordeState CurrentState = EHordeState::Idle;

    UPROPERTY(BlueprintReadOnly)
    EHordeFormation CurrentFormation = EHordeFormation::None;

    UPROPERTY(BlueprintReadOnly)
    FVector HordeCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    AActor* PrimaryTarget = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float FormationRadius = 500.0f;

    UPROPERTY(BlueprintReadOnly)
    float LastUpdateTime = 0.0f;

    FHordeGroup()
    {
        HordeID = -1;
        Leader = nullptr;
        CurrentState = EHordeState::Idle;
        CurrentFormation = EHordeFormation::None;
        HordeCenter = FVector::ZeroVector;
        PrimaryTarget = nullptr;
        FormationRadius = 500.0f;
        LastUpdateTime = 0.0f;
    }
};

/**
 * Horde spawn configuration
 */
USTRUCT(BlueprintType)
struct FHordeSpawnConfig {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<APawn> AIClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SpawnCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHordeFormation InitialFormation = EHordeFormation::Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAutoEngageNearestTarget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float InitialDifficulty = 1.0f;

    FHordeSpawnConfig()
    {
        AIClass = nullptr;
        SpawnCount = 5;
        SpawnRadius = 500.0f;
        InitialFormation = EHordeFormation::Line;
        bAutoEngageNearestTarget = true;
        InitialDifficulty = 1.0f;
    }
};

/**
 * Global Horde Manager for Portal AI
 *
 * Singleton subsystem that manages all AI hordes in the game
 * Handles horde creation, coordination, and high-level tactics
 */
UCLASS()
class PORTAL_API UPortalAIHordeManager : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

protected:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    int32 MaxActiveHordes = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    int32 MaxTotalAI = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    float MaxHordeSize = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    float FormationSpacing = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    float HordeMergeDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    float HordeSplitDistance = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    bool bAutoMergeHordes = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    bool bAutoSplitHordes = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Manager Config")
    float UpdateInterval = 0.5f;

    // State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde Manager State")
    TMap<int32, FHordeGroup> ActiveHordes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde Manager State")
    int32 TotalActiveAI = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde Manager State")
    int32 NextHordeID = 1;

public:
    // Singleton access
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager", meta = (WorldContext = "WorldContextObject"))
    static UPortalAIHordeManager* GetHordeManager(const UObject* WorldContextObject);

    // Horde creation and management
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    int32 CreateHorde(const TArray<UPortalAIHordeComponent*>& InitialMembers);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool DestroyHorde(int32 HordeID);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool MergeHordes(int32 HordeID1, int32 HordeID2);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    TArray<int32> SplitHorde(int32 HordeID, int32 NumGroups = 2);

    // Member management
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool RegisterAI(UPortalAIHordeComponent* AIComponent);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool UnregisterAI(UPortalAIHordeComponent* AIComponent);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool AddToHorde(UPortalAIHordeComponent* AIComponent, int32 HordeID);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool RemoveFromHorde(UPortalAIHordeComponent* AIComponent, int32 HordeID);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool TransferMember(UPortalAIHordeComponent* AIComponent, int32 FromHordeID, int32 ToHordeID);

    // Leadership
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool AssignLeader(int32 HordeID, UPortalAIHordeComponent* Leader);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    UPortalAIHordeComponent* GetHordeLeader(int32 HordeID) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void AutoAssignLeader(int32 HordeID);

    // Formation and movement
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void SetHordeFormation(int32 HordeID, EHordeFormation Formation);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void MoveHordeTo(int32 HordeID, const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void UpdateFormationPositions(int32 HordeID);

    // Combat coordination
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void EngageTarget(int32 HordeID, AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void DisengageHorde(int32 HordeID);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void CoordinateMultiHordeAttack(const TArray<int32>& HordeIDs, AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void InitiateRetreat(int32 HordeID, const FVector& RetreatLocation);

    // Commands
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void BroadcastCommandToHorde(int32 HordeID, const FHordeCommand& Command);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void BroadcastCommandToAll(const FHordeCommand& Command);

    // Spawning
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager", meta = (WorldContext = "WorldContextObject"))
    int32 SpawnHorde(const UObject* WorldContextObject, const FVector& SpawnLocation, const FHordeSpawnConfig& Config);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    void DespawnHorde(int32 HordeID);

    // Queries
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    FHordeGroup GetHordeInfo(int32 HordeID) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    TArray<int32> GetAllHordeIDs() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    int32 GetHordeSize(int32 HordeID) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    TArray<UPortalAIHordeComponent*> GetHordeMembers(int32 HordeID) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    int32 FindNearestHorde(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    bool IsValidHorde(int32 HordeID) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    int32 GetTotalActiveAI() const { return TotalActiveAI; }

    // Analysis
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    float GetHordeThreatLevel(int32 HordeID) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    float GetCombinedThreatLevel() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    FVector GetHordeCenter(int32 HordeID) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde Manager")
    float GetHordeSpread(int32 HordeID) const;

    // Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde Manager")
    void OnHordeCreated(int32 HordeID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde Manager")
    void OnHordeDestroyed(int32 HordeID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde Manager")
    void OnHordesMerged(int32 ResultingHordeID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde Manager")
    void OnHordeSplit(const TArray<int32>& NewHordeIDs);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde Manager")
    void OnHordeEngaged(int32 HordeID, AActor* Target);

private:
    // Update timer
    FTimerHandle UpdateTimerHandle;

    // Internal methods
    void UpdateHordes();
    void ProcessHordeMerging();
    void ProcessHordeSplitting();
    void UpdateHordeStates();
    void CleanupInvalidMembers();
    int32 GenerateHordeID();
    void RecalculateHordeCenter(int32 HordeID);
    void AssignFormationPositions(int32 HordeID);
    bool ShouldMergeHordes(int32 HordeID1, int32 HordeID2) const;
    bool ShouldSplitHorde(int32 HordeID) const;
    void ValidateHordeIntegrity(int32 HordeID);
};