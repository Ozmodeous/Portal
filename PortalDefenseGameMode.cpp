#include "PortalDefenseGameMode.h"
#include "AIOverlordManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PortalCore.h"
#include "PortalDefenseGameState.h"
#include "PortalDefenseSpawner.h"

APortalDefenseGameMode::APortalDefenseGameMode()
{
    GameStateClass = APortalDefenseGameState::StaticClass();

    CaptureZoneRadius = 500.0f;
    TimeToCapture = 60.0f;
    CaptureProgressDecayRate = 0.5f;

    bCaptureActive = false;
    CaptureProgress = 0.0f;
    bPortalCaptured = false;

    PrimaryActorTick.bCanEverTick = true;
}

void APortalDefenseGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("PortalDefenseGameMode BeginPlay"));

    AIOverlord = UAIOverlordManager::GetInstance(GetWorld());
    FindPortalCore();
}

void APortalDefenseGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    CheckPlayersInCaptureZone();
    UpdateCaptureProgress(DeltaTime);
}

void APortalDefenseGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
}

void APortalDefenseGameMode::StartCapture(APawn* Player)
{
    if (!PlayersInZone.Contains(Player)) {
        PlayersInZone.Add(Player);
        OnPlayerEnterCaptureZone.Broadcast(Player);

        if (!bCaptureActive) {
            bCaptureActive = true;
        }
    }
}

void APortalDefenseGameMode::StopCapture(APawn* Player)
{
    if (PlayersInZone.Contains(Player)) {
        PlayersInZone.Remove(Player);
        OnPlayerExitCaptureZone.Broadcast(Player);

        if (PlayersInZone.Num() == 0) {
            bCaptureActive = false;
        }
    }
}

void APortalDefenseGameMode::CompleteCapture()
{
    if (bPortalCaptured) {
        return;
    }

    bPortalCaptured = true;
    bCaptureActive = false;
    CaptureProgress = 1.0f;

    // Stop portal spawning
    if (PortalSpawner) {
        PortalSpawner->StopDefenseSpawning();
    }

    OnPortalCaptured.Broadcast();
}

bool APortalDefenseGameMode::IsPlayerInCaptureZone(APawn* Player) const
{
    if (!Player || !PortalCore) {
        return false;
    }

    float DistanceToPortal = FVector::Dist(Player->GetActorLocation(), PortalCore->GetActorLocation());
    return DistanceToPortal <= CaptureZoneRadius;
}

void APortalDefenseGameMode::FindPortalCore()
{
    PortalCore = Cast<APortalCore>(UGameplayStatics::GetActorOfClass(GetWorld(), APortalCore::StaticClass()));
    if (PortalCore) {
        UE_LOG(LogTemp, Warning, TEXT("Found PortalCore at: %s"), *PortalCore->GetActorLocation().ToString());

        // Find the portal's spawner component
        PortalSpawner = PortalCore->FindComponentByClass<UPortalDefenseSpawner>();
        if (PortalSpawner) {
            UE_LOG(LogTemp, Warning, TEXT("Found PortalDefenseSpawner component"));
        } else {
            UE_LOG(LogTemp, Error, TEXT("No PortalDefenseSpawner component found on Portal"));
        }
    } else {
        UE_LOG(LogTemp, Error, TEXT("No PortalCore in level!"));
    }
}

void APortalDefenseGameMode::UpdateCaptureProgress(float DeltaTime)
{
    if (bPortalCaptured) {
        return;
    }

    if (bCaptureActive && PlayersInZone.Num() > 0) {
        float ProgressIncrease = (DeltaTime / TimeToCapture);
        CaptureProgress = FMath::Clamp(CaptureProgress + ProgressIncrease, 0.0f, 1.0f);
        OnCaptureProgress.Broadcast(CaptureProgress);

        if (CaptureProgress >= 1.0f) {
            CompleteCapture();
        }
    } else if (CaptureProgress > 0.0f) {
        float ProgressDecrease = CaptureProgressDecayRate * DeltaTime;
        CaptureProgress = FMath::Max(0.0f, CaptureProgress - ProgressDecrease);
        OnCaptureProgress.Broadcast(CaptureProgress);
    }
}

void APortalDefenseGameMode::CheckPlayersInCaptureZone()
{
    if (bPortalCaptured) {
        return;
    }

    TArray<AActor*> AllPlayers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), AllPlayers);

    TArray<TObjectPtr<APawn>> CurrentPlayersInZone;

    for (AActor* Actor : AllPlayers) {
        if (APawn* Player = Cast<APawn>(Actor)) {
            if (Player->IsPlayerControlled() && IsPlayerInCaptureZone(Player)) {
                CurrentPlayersInZone.Add(Player);

                if (!PlayersInZone.Contains(Player)) {
                    StartCapture(Player);
                }
            }
        }
    }

    TArray<TObjectPtr<APawn>> PlayersToRemove;
    for (APawn* Player : PlayersInZone) {
        if (!CurrentPlayersInZone.Contains(Player)) {
            PlayersToRemove.Add(Player);
        }
    }

    for (APawn* Player : PlayersToRemove) {
        StopCapture(Player);
    }
}