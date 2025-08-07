// PortalAIFOVComponent.cpp
#include "PortalAIFOVComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PortalACFAIController.h"
#include "WorldCollision.h"

UPortalAIFOVComponent::UPortalAIFOVComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Set default LOD distances
    LODFullDistance = 500.0f;
    LODMediumDistance = 1000.0f;
    LODLowDistance = 2000.0f;
    LODCullDistance = 3000.0f;

    // Set default update rates
    LODFullUpdateRate = 10.0f;
    LODMediumUpdateRate = 4.0f;
    LODLowUpdateRate = 1.0f;
}

void UPortalAIFOVComponent::BeginPlay()
{
    Super::BeginPlay();

    UpdateInterval = 1.0f / LODFullUpdateRate;
}

void UPortalAIFOVComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearAllTargets();
    Super::EndPlay(EndPlayReason);
}

void UPortalAIFOVComponent::Initialize(APortalACFAIController* Controller)
{
    OwnerController = Controller;

    if (OwnerController) {
        OwnerPawn = OwnerController->GetPawn();
        UpdateLODBasedOnPlayerDistance();
    }
}

void UPortalAIFOVComponent::InitializeFOV(APawn* InPawn)
{
    OwnerPawn = InPawn;

    if (OwnerPawn && OwnerController) {
        UpdateLODBasedOnPlayerDistance();
        UE_LOG(LogTemp, Log, TEXT("FOV Component initialized for %s"), *OwnerPawn->GetName());
    }
}

void UPortalAIFOVComponent::UpdateFOV(float DeltaTime)
{
    if (!bIsEnabled || !OwnerPawn) {
        return;
    }

    // Check if we should update based on LOD
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastUpdateTime < UpdateInterval) {
        return;
    }

    LastUpdateTime = CurrentTime;

    // Update LOD based on player distance
    UpdateLODBasedOnPlayerDistance();

    // Process visibility checks based on LOD
    if (CurrentLOD != EFOVLODLevel::LOD_Culled) {
        BatchProcessTargets(DeltaTime);
    }

    // Clean up old visibility data
    CleanupOldData();
}

void UPortalAIFOVComponent::RegisterPotentialTarget(AActor* Target)
{
    if (!Target || VisibilityMap.Contains(Target)) {
        return;
    }

    FTargetVisibilityInfo NewInfo;
    NewInfo.Target = Target;
    NewInfo.Distance = FVector::Dist(OwnerPawn->GetActorLocation(), Target->GetActorLocation());
    NewInfo.LastKnownLocation = Target->GetActorLocation();

    VisibilityMap.Add(Target, NewInfo);
    TargetsToProcess.Add(Target);

    // Update spatial hash if enabled
    if (bUseSpatialHashing) {
        UpdateSpatialHash();
    }
}

void UPortalAIFOVComponent::UnregisterPotentialTarget(AActor* Target)
{
    if (!Target) {
        return;
    }

    VisibilityMap.Remove(Target);
    TargetsToProcess.Remove(Target);

    // Update spatial hash if enabled
    if (bUseSpatialHashing) {
        UpdateSpatialHash();
    }
}

void UPortalAIFOVComponent::ClearAllTargets()
{
    VisibilityMap.Empty();
    TargetsToProcess.Empty();
    SpatialHash.Empty();
}

bool UPortalAIFOVComponent::IsTargetVisible(AActor* Target) const
{
    if (!Target) {
        return false;
    }

    const FTargetVisibilityInfo* Info = VisibilityMap.Find(Target);
    return Info ? Info->bIsVisible : false;
}

float UPortalAIFOVComponent::GetTargetVisibilityScore(AActor* Target) const
{
    if (!Target) {
        return 0.0f;
    }

    const FTargetVisibilityInfo* Info = VisibilityMap.Find(Target);
    return Info ? Info->VisibilityScore : 0.0f;
}

FTargetVisibilityInfo UPortalAIFOVComponent::GetTargetVisibilityInfo(AActor* Target) const
{
    if (!Target) {
        return FTargetVisibilityInfo();
    }

    const FTargetVisibilityInfo* Info = VisibilityMap.Find(Target);
    return Info ? *Info : FTargetVisibilityInfo();
}

TArray<AActor*> UPortalAIFOVComponent::GetVisibleTargets() const
{
    TArray<AActor*> VisibleTargets;

    for (const auto& Pair : VisibilityMap) {
        if (Pair.Value.bIsVisible) {
            VisibleTargets.Add(Pair.Key);
        }
    }

    return VisibleTargets;
}

AActor* UPortalAIFOVComponent::GetClosestVisibleTarget() const
{
    AActor* ClosestTarget = nullptr;
    float ClosestDistance = MAX_FLT;

    for (const auto& Pair : VisibilityMap) {
        if (Pair.Value.bIsVisible && Pair.Value.Distance < ClosestDistance) {
            ClosestTarget = Pair.Key;
            ClosestDistance = Pair.Value.Distance;
        }
    }

    return ClosestTarget;
}

void UPortalAIFOVComponent::SetLODLevel(EFOVLODLevel NewLOD)
{
    if (CurrentLOD != NewLOD) {
        CurrentLOD = NewLOD;

        // Update the update interval based on LOD
        switch (CurrentLOD) {
        case EFOVLODLevel::LOD_Full:
            UpdateInterval = 1.0f / LODFullUpdateRate;
            break;
        case EFOVLODLevel::LOD_Medium:
            UpdateInterval = 1.0f / LODMediumUpdateRate;
            break;
        case EFOVLODLevel::LOD_Low:
            UpdateInterval = 1.0f / LODLowUpdateRate;
            break;
        case EFOVLODLevel::LOD_Culled:
            UpdateInterval = 10.0f; // Very rare updates when culled
            break;
        }

        OnLODChanged(CurrentLOD);
    }
}

EFOVLODLevel UPortalAIFOVComponent::CalculateLODForDistance(float Distance) const
{
    if (Distance < LODFullDistance) {
        return EFOVLODLevel::LOD_Full;
    } else if (Distance < LODMediumDistance) {
        return EFOVLODLevel::LOD_Medium;
    } else if (Distance < LODLowDistance) {
        return EFOVLODLevel::LOD_Low;
    } else {
        return EFOVLODLevel::LOD_Culled;
    }
}

void UPortalAIFOVComponent::UpdateLODBasedOnPlayerDistance()
{
    if (!OwnerPawn) {
        return;
    }

    // Find the closest player
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) {
        return;
    }

    float DistanceToPlayer = FVector::Dist(OwnerPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
    EFOVLODLevel NewLOD = CalculateLODForDistance(DistanceToPlayer);

    SetLODLevel(NewLOD);
}

void UPortalAIFOVComponent::BatchProcessTargets(float DeltaTime)
{
    if (TargetsToProcess.Num() == 0) {
        return;
    }

    // Process a limited number of targets per frame
    int32 TargetsProcessedThisFrame = 0;
    int32 MaxTargetsThisFrame = MaxTargetsPerFrame;

    // Adjust max targets based on LOD
    switch (CurrentLOD) {
    case EFOVLODLevel::LOD_Medium:
        MaxTargetsThisFrame = FMath::Max(1, MaxTargetsPerFrame / 2);
        break;
    case EFOVLODLevel::LOD_Low:
        MaxTargetsThisFrame = 1;
        break;
    }

    // Process targets in a round-robin fashion
    while (TargetsProcessedThisFrame < MaxTargetsThisFrame && TargetsToProcess.Num() > 0) {
        // Wrap around if we've reached the end
        if (CurrentTargetIndex >= TargetsToProcess.Num()) {
            CurrentTargetIndex = 0;
        }

        AActor* Target = TargetsToProcess[CurrentTargetIndex];
        if (IsValid(Target)) {
            ProcessVisibilityForTarget(Target);
        } else {
            // Remove invalid targets
            TargetsToProcess.RemoveAt(CurrentTargetIndex);
            continue;
        }

        CurrentTargetIndex++;
        TargetsProcessedThisFrame++;
    }
}

void UPortalAIFOVComponent::ProcessVisibilityForTarget(AActor* Target)
{
    if (!Target || !OwnerPawn) {
        return;
    }

    FTargetVisibilityInfo* Info = VisibilityMap.Find(Target);
    if (!Info) {
        return;
    }

    bool bWasVisible = Info->bIsVisible;

    // Update distance
    Info->Distance = FVector::Dist(OwnerPawn->GetActorLocation(), Target->GetActorLocation());

    // Check if target is within maximum sight range
    if (Info->Distance > LoseSightRadius) {
        if (Info->bIsVisible) {
            Info->bIsVisible = false;
            OnTargetLostVisibility(Target);
        }
        return;
    }

    // Perform visibility check
    if (PerformVisibilityCheck(Target, *Info)) {
        Info->LastKnownLocation = Target->GetActorLocation();
        Info->LastSeenTime = GetWorld()->GetTimeSeconds();

        if (!bWasVisible) {
            OnTargetBecameVisible(Target);
        }
    } else {
        if (bWasVisible) {
            OnTargetLostVisibility(Target);
        }
    }
}

bool UPortalAIFOVComponent::PerformVisibilityCheck(AActor* Target, FTargetVisibilityInfo& OutInfo)
{
    if (!Target || !OwnerPawn) {
        return false;
    }

    const FVector StartLocation = OwnerPawn->GetActorLocation() + FVector(0, 0, 50); // Eye height offset
    const FVector TargetLocation = Target->GetActorLocation();

    // Check if in FOV
    if (!IsInFieldOfView(TargetLocation)) {
        // Check peripheral vision
        if (bUsePeripheralVision && IsInPeripheralVision(TargetLocation)) {
            OutInfo.bInPeripheralVision = true;
            OutInfo.VisibilityScore = 0.3f; // Lower visibility in peripheral
        } else {
            OutInfo.bIsVisible = false;
            OutInfo.bInPeripheralVision = false;
            OutInfo.VisibilityScore = 0.0f;
            return false;
        }
    } else {
        OutInfo.bInPeripheralVision = false;
    }

    // Perform line trace
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerPawn);
    QueryParams.AddIgnoredActor(Target);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        TargetLocation,
        ECC_Visibility,
        QueryParams);

    // If we didn't hit anything, target is visible
    if (!bHit) {
        OutInfo.bIsVisible = true;
        OutInfo.VisibilityScore = OutInfo.bInPeripheralVision ? 0.3f : 1.0f;
        return true;
    }

    // Calculate partial visibility based on hit location
    OutInfo.VisibilityScore = CalculateVisibilityScore(Target, HitResult);
    OutInfo.bIsVisible = (OutInfo.VisibilityScore > 0.2f);

    return OutInfo.bIsVisible;
}

bool UPortalAIFOVComponent::IsInFieldOfView(const FVector& TargetLocation) const
{
    if (!OwnerPawn) {
        return false;
    }

    const FVector DirectionToTarget = (TargetLocation - OwnerPawn->GetActorLocation()).GetSafeNormal();
    const FVector ForwardVector = OwnerPawn->GetActorForwardVector();

    const float DotProduct = FVector::DotProduct(ForwardVector, DirectionToTarget);
    const float FOVCos = FMath::Cos(FMath::DegreesToRadians(FOVAngle * 0.5f));

    return DotProduct >= FOVCos;
}

bool UPortalAIFOVComponent::IsInPeripheralVision(const FVector& TargetLocation) const
{
    if (!OwnerPawn) {
        return false;
    }

    const FVector DirectionToTarget = (TargetLocation - OwnerPawn->GetActorLocation()).GetSafeNormal();
    const FVector ForwardVector = OwnerPawn->GetActorForwardVector();

    const float DotProduct = FVector::DotProduct(ForwardVector, DirectionToTarget);
    const float PeripheralCos = FMath::Cos(FMath::DegreesToRadians(PeripheralVisionAngle * 0.5f));

    return DotProduct >= PeripheralCos;
}

float UPortalAIFOVComponent::CalculateVisibilityScore(AActor* Target, const FHitResult& HitResult) const
{
    if (!Target) {
        return 0.0f;
    }

    // Basic visibility score based on how much of the target is visible
    // This is a simplified calculation - in production you might want more sophisticated checks

    const float DistanceToHit = HitResult.Distance;
    const float DistanceToTarget = FVector::Dist(OwnerPawn->GetActorLocation(), Target->GetActorLocation());

    if (DistanceToHit >= DistanceToTarget * 0.9f) {
        // Hit is very close to target, partial visibility
        return 0.5f;
    }

    return 0.0f;
}

void UPortalAIFOVComponent::UpdateSpatialHash()
{
    SpatialHash.Empty();

    for (const auto& Pair : VisibilityMap) {
        if (IsValid(Pair.Key)) {
            int32 HashKey = GetSpatialHashKey(Pair.Key->GetActorLocation());

            if (!SpatialHash.Contains(HashKey)) {
                SpatialHash.Add(HashKey, TArray<AActor*>());
            }

            SpatialHash[HashKey].Add(Pair.Key);
        }
    }
}

int32 UPortalAIFOVComponent::GetSpatialHashKey(const FVector& Location) const
{
    int32 X = FMath::FloorToInt(Location.X / SpatialHashCellSize);
    int32 Y = FMath::FloorToInt(Location.Y / SpatialHashCellSize);
    int32 Z = FMath::FloorToInt(Location.Z / SpatialHashCellSize);

    // Simple hash combining
    return X + (Y * 73856093) + (Z * 19349663);
}

TArray<AActor*> UPortalAIFOVComponent::GetNearbyTargets(const FVector& Location, float Radius) const
{
    TArray<AActor*> NearbyTargets;

    if (!bUseSpatialHashing) {
        // Fallback to brute force search
        for (const auto& Pair : VisibilityMap) {
            if (FVector::Dist(Location, Pair.Key->GetActorLocation()) <= Radius) {
                NearbyTargets.Add(Pair.Key);
            }
        }
    } else {
        // Use spatial hash for efficient lookup
        int32 CellRadius = FMath::CeilToInt(Radius / SpatialHashCellSize);
        int32 CenterX = FMath::FloorToInt(Location.X / SpatialHashCellSize);
        int32 CenterY = FMath::FloorToInt(Location.Y / SpatialHashCellSize);
        int32 CenterZ = FMath::FloorToInt(Location.Z / SpatialHashCellSize);

        // Check all cells within radius
        for (int32 X = CenterX - CellRadius; X <= CenterX + CellRadius; X++) {
            for (int32 Y = CenterY - CellRadius; Y <= CenterY + CellRadius; Y++) {
                for (int32 Z = CenterZ - CellRadius; Z <= CenterZ + CellRadius; Z++) {
                    int32 HashKey = X + (Y * 73856093) + (Z * 19349663);

                    if (const TArray<AActor*>* CellTargets = SpatialHash.Find(HashKey)) {
                        for (AActor* Target : *CellTargets) {
                            if (FVector::Dist(Location, Target->GetActorLocation()) <= Radius) {
                                NearbyTargets.Add(Target);
                            }
                        }
                    }
                }
            }
        }
    }

    return NearbyTargets;
}

void UPortalAIFOVComponent::CleanupOldData()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();

    // Remove targets that haven't been seen in a while and are out of range
    TArray<AActor*> TargetsToRemove;

    for (auto& Pair : VisibilityMap) {
        if (!IsValid(Pair.Key)) {
            TargetsToRemove.Add(Pair.Key);
        } else if (!Pair.Value.bIsVisible && CurrentTime - Pair.Value.LastSeenTime > 10.0f && Pair.Value.Distance > LoseSightRadius) {
            TargetsToRemove.Add(Pair.Key);
        }
    }

    for (AActor* Target : TargetsToRemove) {
        UnregisterPotentialTarget(Target);
    }
}