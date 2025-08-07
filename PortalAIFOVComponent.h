// PortalAIFOVComponent.h
#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PortalAIFOVComponent.generated.h"

// Forward declarations
class APortalACFAIController;
class UAIPerceptionComponent;

/**
 * LOD levels for FOV processing
 */
UENUM(BlueprintType)
enum class EFOVLODLevel : uint8 {
    LOD_Full UMETA(DisplayName = "Full Detail"), // Close range, full updates
    LOD_Medium UMETA(DisplayName = "Medium Detail"), // Medium range, reduced updates
    LOD_Low UMETA(DisplayName = "Low Detail"), // Far range, minimal updates
    LOD_Culled UMETA(DisplayName = "Culled") // Out of range, no updates
};

/**
 * Visibility information for a single target
 */
USTRUCT(BlueprintType)
struct FTargetVisibilityInfo {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AActor* Target = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float Distance = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float LastSeenTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FVector LastKnownLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    bool bIsVisible = false;

    UPROPERTY(BlueprintReadOnly)
    bool bInPeripheralVision = false;

    UPROPERTY(BlueprintReadOnly)
    float VisibilityScore = 0.0f; // 0-1 value for partial visibility

    FTargetVisibilityInfo()
    {
        Target = nullptr;
        Distance = 0.0f;
        LastSeenTime = 0.0f;
        LastKnownLocation = FVector::ZeroVector;
        bIsVisible = false;
        bInPeripheralVision = false;
        VisibilityScore = 0.0f;
    }
};

/**
 * Optimized FOV Component for Portal AI
 *
 * Handles efficient field of view calculations for hundreds of AI entities
 * Uses LOD system and spatial hashing for performance optimization
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UPortalAIFOVComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UPortalAIFOVComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FOV Config")
    float SightRadius = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FOV Config")
    float PeripheralVisionAngle = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FOV Config")
    float FOVAngle = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FOV Config")
    float LoseSightRadius = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FOV Config")
    bool bUsePeripheralVision = true;

    // LOD Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Config")
    float LODFullDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Config")
    float LODMediumDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Config")
    float LODLowDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Config")
    float LODCullDistance = 3000.0f;

    // Update rates per LOD (checks per second)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Config")
    float LODFullUpdateRate = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Config")
    float LODMediumUpdateRate = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Config")
    float LODLowUpdateRate = 1.0f;

    // Performance settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance")
    int32 MaxTargetsPerFrame = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance")
    bool bUseSpatialHashing = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance")
    float SpatialHashCellSize = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance")
    bool bUseAsyncLineTraces = true;

    // State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FOV State")
    TMap<AActor*, FTargetVisibilityInfo> VisibilityMap;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FOV State")
    EFOVLODLevel CurrentLOD = EFOVLODLevel::LOD_Full;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FOV State")
    bool bIsEnabled = true;

public:
    // Initialization
    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void Initialize(APortalACFAIController* Controller);

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void InitializeFOV(APawn* InPawn);

    // Main update
    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void UpdateFOV(float DeltaTime);

    // Target management
    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void RegisterPotentialTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void UnregisterPotentialTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void ClearAllTargets();

    // Visibility queries
    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    bool IsTargetVisible(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    float GetTargetVisibilityScore(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    FTargetVisibilityInfo GetTargetVisibilityInfo(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    TArray<AActor*> GetVisibleTargets() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    AActor* GetClosestVisibleTarget() const;

    // LOD management
    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void SetLODLevel(EFOVLODLevel NewLOD);

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    EFOVLODLevel CalculateLODForDistance(float Distance) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void UpdateLODBasedOnPlayerDistance();

    // Performance control
    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void SetEnabled(bool bEnable) { bIsEnabled = bEnable; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    bool IsEnabled() const { return bIsEnabled; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void SetSightRadius(float NewRadius) { SightRadius = NewRadius; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI FOV")
    void SetFOVAngle(float NewAngle) { FOVAngle = NewAngle; }

    // Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI FOV")
    void OnTargetBecameVisible(AActor* Target);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI FOV")
    void OnTargetLostVisibility(AActor* Target);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI FOV")
    void OnLODChanged(EFOVLODLevel NewLOD);

private:
    UPROPERTY()
    APortalACFAIController* OwnerController;

    UPROPERTY()
    APawn* OwnerPawn;

    // Performance tracking
    float LastUpdateTime = 0.0f;
    float UpdateInterval = 0.1f;
    int32 CurrentTargetIndex = 0;
    TArray<AActor*> TargetsToProcess;

    // Spatial hashing
    TMap<int32, TArray<AActor*>> SpatialHash;

    // Internal methods
    void ProcessVisibilityForTarget(AActor* Target);
    bool PerformVisibilityCheck(AActor* Target, FTargetVisibilityInfo& OutInfo);
    bool IsInFieldOfView(const FVector& TargetLocation) const;
    bool IsInPeripheralVision(const FVector& TargetLocation) const;
    float CalculateVisibilityScore(AActor* Target, const FHitResult& HitResult) const;

    void UpdateSpatialHash();
    int32 GetSpatialHashKey(const FVector& Location) const;
    TArray<AActor*> GetNearbyTargets(const FVector& Location, float Radius) const;

    void BatchProcessTargets(float DeltaTime);
    void AsyncLineTraceCallback(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
    void CleanupOldData();
};