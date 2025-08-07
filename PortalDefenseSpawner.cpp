#include "PortalDefenseSpawner.h"
#include "ACFAIController.h"
#include "AIOverlordManager.h"
#include "AIOverseenComponent.h"
#include "Components/ACFTeamManagerComponent.h"
#include "Engine/World.h"
#include "Game/ACFFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "PortalCore.h"
#include "PortalDefenseAIController.h"

UPortalDefenseSpawner::UPortalDefenseSpawner()
{
    PrimaryComponentTick.bCanEverTick = false;

    bAutoStartOnBeginPlay = true;
    SpawnCheckInterval = 10.0f;
    MaxGroundSearchDistance = 2000.0f;
    GroundOffset = 50.0f;
    bReplaceDeadGuards = true;
    bSpawningActive = false;

    // 10 Defense Rings Setup

    // Ring 1 - Inner Defense
    FDefenseRingConfig Ring1;
    Ring1.RingType = ESpawnRingType::InnerDefense;
    Ring1.GuardsPerRing = 4;
    Ring1.RingDistance = 600.0f;
    Ring1.PatrolRadius = 150.0f;
    Ring1.bPatrolAroundPortal = true;

    // Ring 2 - Inner Defense
    FDefenseRingConfig Ring2;
    Ring2.RingType = ESpawnRingType::InnerDefense;
    Ring2.GuardsPerRing = 6;
    Ring2.RingDistance = 1000.0f;
    Ring2.PatrolRadius = 200.0f;
    Ring2.bPatrolAroundPortal = true;

    // Ring 3 - Middle Patrol
    FDefenseRingConfig Ring3;
    Ring3.RingType = ESpawnRingType::MiddlePatrol;
    Ring3.GuardsPerRing = 6;
    Ring3.RingDistance = 1500.0f;
    Ring3.PatrolRadius = 300.0f;
    Ring3.bPatrolAroundPortal = false;

    // Ring 4 - Middle Patrol
    FDefenseRingConfig Ring4;
    Ring4.RingType = ESpawnRingType::MiddlePatrol;
    Ring4.GuardsPerRing = 8;
    Ring4.RingDistance = 2200.0f;
    Ring4.PatrolRadius = 400.0f;
    Ring4.bPatrolAroundPortal = false;

    // Ring 5 - Middle Patrol
    FDefenseRingConfig Ring5;
    Ring5.RingType = ESpawnRingType::MiddlePatrol;
    Ring5.GuardsPerRing = 8;
    Ring5.RingDistance = 3000.0f;
    Ring5.PatrolRadius = 500.0f;
    Ring5.bPatrolAroundPortal = false;

    // Ring 6 - Outer Patrol
    FDefenseRingConfig Ring6;
    Ring6.RingType = ESpawnRingType::OuterPatrol;
    Ring6.GuardsPerRing = 10;
    Ring6.RingDistance = 4000.0f;
    Ring6.PatrolRadius = 600.0f;
    Ring6.bPatrolAroundPortal = false;

    // Ring 7 - Outer Patrol
    FDefenseRingConfig Ring7;
    Ring7.RingType = ESpawnRingType::OuterPatrol;
    Ring7.GuardsPerRing = 12;
    Ring7.RingDistance = 5500.0f;
    Ring7.PatrolRadius = 700.0f;
    Ring7.bPatrolAroundPortal = false;

    // Ring 8 - Outer Patrol
    FDefenseRingConfig Ring8;
    Ring8.RingType = ESpawnRingType::OuterPatrol;
    Ring8.GuardsPerRing = 12;
    Ring8.RingDistance = 7500.0f;
    Ring8.PatrolRadius = 800.0f;
    Ring8.bPatrolAroundPortal = false;

    // Ring 9 - Perimeter Watch
    FDefenseRingConfig Ring9;
    Ring9.RingType = ESpawnRingType::PerimeterWatch;
    Ring9.GuardsPerRing = 14;
    Ring9.RingDistance = 10000.0f;
    Ring9.PatrolRadius = 1000.0f;
    Ring9.bPatrolAroundPortal = false;

    // Ring 10 - Perimeter Watch
    FDefenseRingConfig Ring10;
    Ring10.RingType = ESpawnRingType::PerimeterWatch;
    Ring10.GuardsPerRing = 16;
    Ring10.RingDistance = 15000.0f;
    Ring10.PatrolRadius = 1200.0f;
    Ring10.bPatrolAroundPortal = false;

    DefenseRings.Add(Ring1);
    DefenseRings.Add(Ring2);
    DefenseRings.Add(Ring3);
    DefenseRings.Add(Ring4);
    DefenseRings.Add(Ring5);
    DefenseRings.Add(Ring6);
    DefenseRings.Add(Ring7);
    DefenseRings.Add(Ring8);
    DefenseRings.Add(Ring9);
    DefenseRings.Add(Ring10);
}

void UPortalDefenseSpawner::BeginPlay()
{
    Super::BeginPlay();

    InitializePortalReference();
    RegisterWithOverlord();

    if (bAutoStartOnBeginPlay) {
        // Delay spawn to ensure everything is initialized
        FTimerHandle DelayHandle;
        GetWorld()->GetTimerManager().SetTimer(DelayHandle, this, &UPortalDefenseSpawner::StartDefenseSpawning, 1.0f, false);
    }
}

void UPortalDefenseSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopDefenseSpawning();
    DespawnAllGuards();

    GetWorld()->GetTimerManager().ClearTimer(SpawnCheckTimer);
    for (auto& TimerPair : RespawnTimers) {
        GetWorld()->GetTimerManager().ClearTimer(TimerPair.Value);
    }
    RespawnTimers.Empty();

    Super::EndPlay(EndPlayReason);
}

void UPortalDefenseSpawner::StartDefenseSpawning()
{
    if (!PortalCore) {
        UE_LOG(LogTemp, Error, TEXT("PortalDefenseSpawner: No PortalCore found"));
        return;
    }

    bSpawningActive = true;
    SpawnAllRings();

    // Start periodic check for missing guards
    if (bReplaceDeadGuards) {
        GetWorld()->GetTimerManager().SetTimer(SpawnCheckTimer, this, &UPortalDefenseSpawner::OnSpawnCheckTimer, SpawnCheckInterval, true);
    }

    UE_LOG(LogTemp, Warning, TEXT("Portal Defense Spawning Started - %d rings configured"), DefenseRings.Num());
}

void UPortalDefenseSpawner::StopDefenseSpawning()
{
    bSpawningActive = false;
    GetWorld()->GetTimerManager().ClearTimer(SpawnCheckTimer);

    UE_LOG(LogTemp, Warning, TEXT("Portal Defense Spawning Stopped"));
}

void UPortalDefenseSpawner::SpawnAllRings()
{
    for (int32 RingIndex = 0; RingIndex < DefenseRings.Num(); RingIndex++) {
        if (DefenseRings[RingIndex].bSpawnOnStart) {
            SpawnRing(RingIndex);
        }
    }
}

void UPortalDefenseSpawner::SpawnRing(int32 RingIndex)
{
    if (!DefenseRings.IsValidIndex(RingIndex)) {
        UE_LOG(LogTemp, Error, TEXT("Invalid ring index: %d"), RingIndex);
        return;
    }

    const FDefenseRingConfig& RingConfig = DefenseRings[RingIndex];

    if (!RingConfig.GuardClass) {
        UE_LOG(LogTemp, Error, TEXT("No GuardClass set for ring %d"), RingIndex);
        return;
    }

    for (int32 PositionIndex = 0; PositionIndex < RingConfig.GuardsPerRing; PositionIndex++) {
        SpawnGuardAtPosition(RingConfig, RingIndex, PositionIndex);
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawned ring %d with %d guards at distance %.1f"),
        RingIndex, RingConfig.GuardsPerRing, RingConfig.RingDistance);
}

APawn* UPortalDefenseSpawner::SpawnGuardAtPosition(const FDefenseRingConfig& RingConfig, int32 RingIndex, int32 PositionIndex)
{
    FVector RingPosition = GetRingPosition(RingConfig.RingDistance, PositionIndex, RingConfig.GuardsPerRing);
    FVector SpawnLocation = FindGroundAtPosition(RingPosition);

    if (SpawnLocation.IsZero()) {
        UE_LOG(LogTemp, Error, TEXT("Failed to find ground at ring position"));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APawn* SpawnedGuard = GetWorld()->SpawnActor<APawn>(RingConfig.GuardClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
    if (!SpawnedGuard) {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn guard"));
        return nullptr;
    }

    // Setup guard behavior
    FVector PatrolCenter = GetPatrolCenter(RingConfig, SpawnLocation);
    SetupGuardBehavior(SpawnedGuard, RingConfig, SpawnLocation, PatrolCenter);

    // Track the guard
    FActiveGuardInfo GuardInfo;
    GuardInfo.GuardPawn = SpawnedGuard;
    GuardInfo.RingIndex = RingIndex;
    GuardInfo.PositionIndex = PositionIndex;
    GuardInfo.SpawnLocation = SpawnLocation;
    GuardInfo.RingConfig = RingConfig;

    ActiveGuards.Add(GuardInfo);

    // Bind destruction event
    SpawnedGuard->OnDestroyed.AddDynamic(this, &UPortalDefenseSpawner::OnGuardDestroyed);

    UE_LOG(LogTemp, Log, TEXT("Spawned guard at ring %d, position %d (%.1f, %.1f, %.1f)"),
        RingIndex, PositionIndex, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);

    return SpawnedGuard;
}

void UPortalDefenseSpawner::ScheduleGuardRespawn(const FActiveGuardInfo& GuardInfo)
{
    FGuid RespawnID = FGuid::NewGuid();
    FTimerHandle& RespawnTimer = RespawnTimers.Add(RespawnID);

    FTimerDelegate RespawnDelegate;
    RespawnDelegate.BindUFunction(this, FName("OnRespawnTimerComplete"), RespawnID, GuardInfo);

    GetWorld()->GetTimerManager().SetTimer(RespawnTimer, RespawnDelegate, GuardInfo.RingConfig.RespawnDelay, false);

    UE_LOG(LogTemp, Log, TEXT("Scheduled respawn for ring %d, position %d in %.1f seconds"),
        GuardInfo.RingIndex, GuardInfo.PositionIndex, GuardInfo.RingConfig.RespawnDelay);
}

FVector UPortalDefenseSpawner::GetRingPosition(float Distance, int32 PositionIndex, int32 TotalPositions) const
{
    if (!PortalCore || TotalPositions <= 0) {
        return FVector::ZeroVector;
    }

    float AngleStep = 360.0f / TotalPositions;
    float Angle = PositionIndex * AngleStep;
    float RadianAngle = FMath::DegreesToRadians(Angle);

    FVector PortalLocation = PortalCore->GetActorLocation();
    FVector Offset = FVector(
        FMath::Cos(RadianAngle) * Distance,
        FMath::Sin(RadianAngle) * Distance,
        0.0f);

    return PortalLocation + Offset;
}

FVector UPortalDefenseSpawner::FindGroundAtPosition(FVector TargetPosition) const
{
    // Simple ground trace from above
    FVector StartLocation = TargetPosition + FVector(0.0f, 0.0f, MaxGroundSearchDistance);
    FVector EndLocation = TargetPosition + FVector(0.0f, 0.0f, -MaxGroundSearchDistance);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.bReturnPhysicalMaterial = false;

    if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_WorldStatic, QueryParams)) {
        return HitResult.Location + FVector(0.0f, 0.0f, GroundOffset);
    }

    // Fallback to target position
    return TargetPosition;
}

void UPortalDefenseSpawner::SetupGuardBehavior(APawn* Guard, const FDefenseRingConfig& RingConfig, FVector SpawnLocation, FVector PatrolCenter)
{
    if (!Guard) {
        return;
    }

    // Setup ACF AI Controller
    if (AACFAIController* ACFController = Cast<AACFAIController>(Guard->GetController())) {
        // Set team
        ACFController->SetCombatTeam(ETeam::ETeam2);

        // Set patrol state
        FGameplayTag PatrolState = UACFFunctionLibrary::GetAIStateTag(EAIState::EPatrol);
        if (PatrolState.IsValid()) {
            ACFController->SetCurrentAIState(PatrolState);
        }

        // Set initial target
        FVector InitialTarget = PatrolCenter + FVector(FMath::RandRange(-RingConfig.PatrolRadius, RingConfig.PatrolRadius), FMath::RandRange(-RingConfig.PatrolRadius, RingConfig.PatrolRadius), 0.0f);
        ACFController->SetTargetLocationBK(InitialTarget);

        // Register with overlord
        if (AIOverlord) {
            AIOverlord->RegisterAI(ACFController);
        }
    }

    // Setup Portal Defense AI Controller
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(Guard->GetController())) {
        PatrolAI->SetPatrolCenter(PatrolCenter);
        PatrolAI->SetPatrolRadius(RingConfig.PatrolRadius);
        PatrolAI->SetPortalTarget(PortalCore);
        PatrolAI->StartPatrolling();
    }

    // Setup AI Overseen Component
    if (UAIOverseenComponent* OverseenComp = Guard->FindComponentByClass<UAIOverseenComponent>()) {
        OverseenComp->SetPatrolCenter(PatrolCenter);
        OverseenComp->SetPatrolRadius(RingConfig.PatrolRadius);
        OverseenComp->SetCombatTeam(ETeam::ETeam2);
    }
}

void UPortalDefenseSpawner::DespawnAllGuards()
{
    for (const FActiveGuardInfo& GuardInfo : ActiveGuards) {
        if (GuardInfo.GuardPawn && IsValid(GuardInfo.GuardPawn)) {
            GuardInfo.GuardPawn->Destroy();
        }
    }

    ActiveGuards.Empty();
    UE_LOG(LogTemp, Warning, TEXT("Despawned all guards"));
}

int32 UPortalDefenseSpawner::GetMaxGuardCount() const
{
    int32 MaxCount = 0;
    for (const FDefenseRingConfig& Ring : DefenseRings) {
        MaxCount += Ring.GuardsPerRing;
    }
    return MaxCount;
}

void UPortalDefenseSpawner::InitializePortalReference()
{
    PortalCore = Cast<APortalCore>(GetOwner());
    if (!PortalCore) {
        UE_LOG(LogTemp, Error, TEXT("PortalDefenseSpawner must be attached to a PortalCore"));
    }
}

void UPortalDefenseSpawner::RegisterWithOverlord()
{
    AIOverlord = UAIOverlordManager::GetInstance(GetWorld());
    if (AIOverlord) {
        UE_LOG(LogTemp, Log, TEXT("Portal Defense Spawner registered with AI Overlord"));
    }
}

void UPortalDefenseSpawner::CheckForMissingGuards()
{
    // Remove invalid guards
    ActiveGuards.RemoveAll([](const FActiveGuardInfo& GuardInfo) {
        return !GuardInfo.GuardPawn || !IsValid(GuardInfo.GuardPawn);
    });

    // Check each ring for missing guards
    for (int32 RingIndex = 0; RingIndex < DefenseRings.Num(); RingIndex++) {
        const FDefenseRingConfig& RingConfig = DefenseRings[RingIndex];

        // Count guards in this ring
        int32 GuardsInRing = 0;
        for (const FActiveGuardInfo& GuardInfo : ActiveGuards) {
            if (GuardInfo.RingIndex == RingIndex) {
                GuardsInRing++;
            }
        }

        // Spawn missing guards
        if (GuardsInRing < RingConfig.GuardsPerRing) {
            int32 MissingCount = RingConfig.GuardsPerRing - GuardsInRing;

            // Find missing positions
            for (int32 PositionIndex = 0; PositionIndex < RingConfig.GuardsPerRing; PositionIndex++) {
                bool bPositionOccupied = false;
                for (const FActiveGuardInfo& GuardInfo : ActiveGuards) {
                    if (GuardInfo.RingIndex == RingIndex && GuardInfo.PositionIndex == PositionIndex) {
                        bPositionOccupied = true;
                        break;
                    }
                }

                if (!bPositionOccupied && MissingCount > 0) {
                    SpawnGuardAtPosition(RingConfig, RingIndex, PositionIndex);
                    MissingCount--;
                }
            }
        }
    }
}

bool UPortalDefenseSpawner::IsPositionValid(FVector Position) const
{
    // Basic validation - ensure position is reasonable distance from portal
    if (!PortalCore) {
        return false;
    }

    float Distance = FVector::Dist2D(Position, PortalCore->GetActorLocation());
    return Distance > 100.0f && Distance < 50000.0f;
}

FVector UPortalDefenseSpawner::GetPatrolCenter(const FDefenseRingConfig& RingConfig, FVector SpawnLocation) const
{
    if (RingConfig.bPatrolAroundPortal && PortalCore) {
        return PortalCore->GetActorLocation();
    }

    return SpawnLocation;
}

void UPortalDefenseSpawner::OnSpawnCheckTimer()
{
    if (bSpawningActive && bReplaceDeadGuards) {
        CheckForMissingGuards();
    }
}

void UPortalDefenseSpawner::OnGuardDestroyed(AActor* DestroyedActor)
{
    if (APawn* DestroyedGuard = Cast<APawn>(DestroyedActor)) {
        // Find and remove the guard from active list
        for (int32 i = 0; i < ActiveGuards.Num(); i++) {
            if (ActiveGuards[i].GuardPawn == DestroyedGuard) {
                FActiveGuardInfo GuardInfo = ActiveGuards[i];
                ActiveGuards.RemoveAt(i);

                // Schedule respawn if enabled
                if (bReplaceDeadGuards && bSpawningActive) {
                    ScheduleGuardRespawn(GuardInfo);
                }

                UE_LOG(LogTemp, Log, TEXT("Guard destroyed at ring %d, position %d"), GuardInfo.RingIndex, GuardInfo.PositionIndex);
                break;
            }
        }

        // Notify overlord
        if (AIOverlord) {
            AIOverlord->RecordAIDeath(Cast<AACFAIController>(DestroyedGuard->GetController()), DestroyedGuard->GetActorLocation());
        }
    }
}

void UPortalDefenseSpawner::OnRespawnTimerComplete(FGuid RespawnID, FActiveGuardInfo GuardInfo)
{
    RespawnTimers.Remove(RespawnID);

    if (bSpawningActive) {
        SpawnGuardAtPosition(GuardInfo.RingConfig, GuardInfo.RingIndex, GuardInfo.PositionIndex);
    }
}