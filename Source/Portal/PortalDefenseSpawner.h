#pragma once

#include "ACFCoreTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "PortalDefenseSpawner.generated.h"

class APortalCore;
class UAIOverlordManager;

UENUM(BlueprintType)
enum class ESpawnRingType : uint8 {
    InnerDefense UMETA(DisplayName = "Inner Defense Ring"),
    MiddlePatrol UMETA(DisplayName = "Middle Patrol Ring"),
    OuterPatrol UMETA(DisplayName = "Outer Patrol Ring"),
    PerimeterWatch UMETA(DisplayName = "Perimeter Watch Ring")
};

USTRUCT(BlueprintType)
struct FDefenseRingConfig {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESpawnRingType RingType = ESpawnRingType::MiddlePatrol;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<APawn> GuardClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GuardsPerRing = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RingDistance = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PatrolRadius = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bPatrolAroundPortal = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RespawnDelay = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSpawnOnStart = true;

    FDefenseRingConfig()
    {
        RingType = ESpawnRingType::MiddlePatrol;
        GuardClass = nullptr;
        GuardsPerRing = 4;
        RingDistance = 1200.0f;
        PatrolRadius = 300.0f;
        bPatrolAroundPortal = false;
        RespawnDelay = 180.0f;
        bSpawnOnStart = true;
    }
};

USTRUCT(BlueprintType)
struct FActiveGuardInfo {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<APawn> GuardPawn;

    UPROPERTY()
    int32 RingIndex;

    UPROPERTY()
    int32 PositionIndex;

    UPROPERTY()
    FVector SpawnLocation;

    UPROPERTY()
    FDefenseRingConfig RingConfig;

    FActiveGuardInfo()
    {
        GuardPawn = nullptr;
        RingIndex = 0;
        PositionIndex = 0;
        SpawnLocation = FVector::ZeroVector;
    }
};

UCLASS(ClassGroup = (Portal), meta = (BlueprintSpawnableComponent))
class PORTAL_API UPortalDefenseSpawner : public UActorComponent {
    GENERATED_BODY()

public:
    UPortalDefenseSpawner();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Spawning Control
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StartDefenseSpawning();

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void StopDefenseSpawning();

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SpawnAllRings();

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SpawnRing(int32 RingIndex);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void DespawnAllGuards();

    // Individual Guard Management
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    APawn* SpawnGuardAtPosition(const FDefenseRingConfig& RingConfig, int32 RingIndex, int32 PositionIndex);

    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void ScheduleGuardRespawn(const FActiveGuardInfo& GuardInfo);

    // Position Calculation
    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    FVector GetRingPosition(float Distance, int32 PositionIndex, int32 TotalPositions) const;

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    FVector FindGroundAtPosition(FVector TargetPosition) const;

    // Guard Setup
    UFUNCTION(BlueprintCallable, Category = "Portal Defense")
    void SetupGuardBehavior(APawn* Guard, const FDefenseRingConfig& RingConfig, FVector SpawnLocation, FVector PatrolCenter);

    // Status
    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    int32 GetActiveGuardCount() const { return ActiveGuards.Num(); }

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    int32 GetMaxGuardCount() const;

    UFUNCTION(BlueprintPure, Category = "Portal Defense")
    bool IsSpawningActive() const { return bSpawningActive; }

protected:
    // Defense Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense Rings", meta = (TitleProperty = "RingType"))
    TArray<FDefenseRingConfig> DefenseRings;

    // Spawning Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    bool bAutoStartOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    float SpawnCheckInterval = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    float MaxGroundSearchDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    float GroundOffset = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    bool bReplaceDeadGuards = true;

    // Portal Reference
    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    TObjectPtr<APortalCore> PortalCore;

    // Active Guards Tracking
    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    TArray<FActiveGuardInfo> ActiveGuards;

    // Spawning State
    UPROPERTY(BlueprintReadOnly, Category = "Portal Defense")
    bool bSpawningActive;

private:
    // Timers
    FTimerHandle SpawnCheckTimer;
    TMap<FGuid, FTimerHandle> RespawnTimers;

    // AI Overlord Integration
    UPROPERTY()
    TObjectPtr<UAIOverlordManager> AIOverlord;

    // Internal Functions
    void InitializePortalReference();
    void RegisterWithOverlord();
    void CheckForMissingGuards();
    bool IsPositionValid(FVector Position) const;
    FVector GetPatrolCenter(const FDefenseRingConfig& RingConfig, FVector SpawnLocation) const;

    UFUNCTION()
    void OnSpawnCheckTimer();

    UFUNCTION()
    void OnGuardDestroyed(AActor* DestroyedActor);

    UFUNCTION()
    void OnRespawnTimerComplete(FGuid RespawnID, FActiveGuardInfo GuardInfo);
};