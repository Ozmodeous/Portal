#include "PortalDefenseGameState.h"
#include "ACFTeamsConfigDataAsset.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PortalCore.h"

APortalDefenseGameState::APortalDefenseGameState()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    // Energy Configuration
    CurrentEnergy = 100;
    StartingEnergy = 100;
    EnergyExtractionRate = 10;
    EnergyExtractionInterval = 1.0f;

    // Capture Information
    CaptureProgress = 0.0f;
    bIsCapturing = false;
    PlayersInCaptureZone = 0;

    // Portal Tracking
    LastPortalHealth = 0.0f;
}

void APortalDefenseGameState::BeginPlay()
{
    Super::BeginPlay();

    CurrentEnergy = StartingEnergy;
    FindPortalCore();
    StartEnergyExtraction();
}

void APortalDefenseGameState::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdatePortalHealthTracking();
}

void APortalDefenseGameState::AddEnergy(int32 Amount)
{
    if (HasAuthority()) {
        CurrentEnergy += Amount;
        OnEnergyChanged.Broadcast(CurrentEnergy);
    }
}

bool APortalDefenseGameState::SpendEnergy(int32 Amount)
{
    if (!HasAuthority()) {
        return false;
    }

    if (CurrentEnergy >= Amount) {
        CurrentEnergy -= Amount;
        OnEnergyChanged.Broadcast(CurrentEnergy);
        return true;
    }

    return false;
}

void APortalDefenseGameState::ExtractEnergyFromPortal()
{
    if (!HasAuthority() || !PortalCore || PortalCore->IsDestroyed()) {
        return;
    }

    // Award energy based on portal extraction efficiency
    int32 ExtractedEnergy = PortalCore->ExtractEnergy();
    AddEnergy(ExtractedEnergy);
}

void APortalDefenseGameState::SetCaptureProgress(float Progress)
{
    if (HasAuthority()) {
        float OldProgress = CaptureProgress;
        CaptureProgress = FMath::Clamp(Progress, 0.0f, 1.0f);

        if (FMath::Abs(CaptureProgress - OldProgress) > 0.01f) {
            OnCaptureProgressChanged.Broadcast(CaptureProgress);
        }
    }
}

void APortalDefenseGameState::SetCapturing(bool bCapturing)
{
    if (HasAuthority()) {
        bIsCapturing = bCapturing;
    }
}

void APortalDefenseGameState::SetPlayersInZone(int32 PlayerCount)
{
    if (HasAuthority()) {
        PlayersInCaptureZone = PlayerCount;
    }
}

float APortalDefenseGameState::GetPortalHealthPercent() const
{
    if (!PortalCore) {
        return 0.0f;
    }

    return PortalCore->GetHealthPercent();
}

bool APortalDefenseGameState::IsPortalDestroyed() const
{
    if (!PortalCore) {
        return true;
    }

    return PortalCore->IsDestroyed();
}

void APortalDefenseGameState::RegisterEnemy(APawn* Guard)
{
    if (Guard && !ActiveGuards.Contains(Guard)) {
        ActiveGuards.Add(Guard);

        // Bind to destruction
        Guard->OnDestroyed.AddDynamic(this, &APortalDefenseGameState::OnGuardDestroyed);

        OnPatrolGuardCountChanged.Broadcast(ActiveGuards.Num());
    }
}

void APortalDefenseGameState::UnregisterEnemy(APawn* Guard)
{
    if (Guard) {
        ActiveGuards.Remove(Guard);
        OnPatrolGuardCountChanged.Broadcast(ActiveGuards.Num());
    }
}

void APortalDefenseGameState::OnGuardDestroyed(AActor* DestroyedActor)
{
    if (APawn* Guard = Cast<APawn>(DestroyedActor)) {
        UnregisterEnemy(Guard);
    }
}

void APortalDefenseGameState::FindPortalCore()
{
    PortalCore = Cast<APortalCore>(UGameplayStatics::GetActorOfClass(GetWorld(), APortalCore::StaticClass()));

    if (PortalCore) {
        LastPortalHealth = PortalCore->GetCurrentHealth();
    }
}

void APortalDefenseGameState::StartEnergyExtraction()
{
    if (HasAuthority()) {
        GetWorldTimerManager().SetTimer(EnergyExtractionTimer, this, &APortalDefenseGameState::ExtractEnergyTick, EnergyExtractionInterval, true);
    }
}

void APortalDefenseGameState::ExtractEnergyTick()
{
    if (!PortalCore || PortalCore->IsDestroyed()) {
        GetWorldTimerManager().ClearTimer(EnergyExtractionTimer);
        return;
    }

    // Passive energy extraction while portal is active
    AddEnergy(EnergyExtractionRate);
}

void APortalDefenseGameState::UpdatePortalHealthTracking()
{
    if (!PortalCore) {
        return;
    }

    float CurrentHealth = PortalCore->GetCurrentHealth();
    float MaxHealth = PortalCore->GetMaxHealth();

    if (FMath::Abs(CurrentHealth - LastPortalHealth) > 0.1f) {
        LastPortalHealth = CurrentHealth;
        OnPortalHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    }
}

void APortalDefenseGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APortalDefenseGameState, CurrentEnergy);
    DOREPLIFETIME(APortalDefenseGameState, CaptureProgress);
    DOREPLIFETIME(APortalDefenseGameState, bIsCapturing);
    DOREPLIFETIME(APortalDefenseGameState, PlayersInCaptureZone);
}