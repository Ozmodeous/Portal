#include "PortalInteractionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "PortalCore.h"
#include "PortalDefenseGameMode.h"
#include "PortalDefenseGameState.h"

UPortalInteractionComponent::UPortalInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bWasInCaptureZone = false;
    bHasShownCaptureZoneIndicator = false;
}

void UPortalInteractionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (APortalDefenseGameState* GameState = GetPortalGameState()) {
        GameState->OnEnergyChanged.AddDynamic(this, &UPortalInteractionComponent::OnEnergyChanged);
        GameState->OnPortalHealthChanged.AddDynamic(this, &UPortalInteractionComponent::OnPortalHealthChanged);
        GameState->OnCaptureProgressChanged.AddDynamic(this, &UPortalInteractionComponent::OnCaptureProgressChanged);
        GameState->OnPatrolGuardCountChanged.AddDynamic(this, &UPortalInteractionComponent::OnPatrolGuardCountChanged);
    }

    UpdateUI();
}

void UPortalInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateUI();
    CheckCaptureZoneStatus();
}

void UPortalInteractionComponent::EnterCaptureZone()
{
    if (APortalDefenseGameMode* GameMode = GetWorld()->GetAuthGameMode<APortalDefenseGameMode>()) {
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) {
            GameMode->StartCapture(OwnerPawn);
        }
    }

    ShowCaptureZoneIndicator(true);
    UE_LOG(LogTemp, Log, TEXT("Player entered capture zone"));
}

void UPortalInteractionComponent::ExitCaptureZone()
{
    if (APortalDefenseGameMode* GameMode = GetWorld()->GetAuthGameMode<APortalDefenseGameMode>()) {
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) {
            GameMode->StopCapture(OwnerPawn);
        }
    }

    ShowCaptureZoneIndicator(false);
    UE_LOG(LogTemp, Log, TEXT("Player exited capture zone"));
}

void UPortalInteractionComponent::ForceCompletCapture()
{
    if (APortalDefenseGameMode* GameMode = GetWorld()->GetAuthGameMode<APortalDefenseGameMode>()) {
        GameMode->CompleteCapture();
    }
}

APortalDefenseGameState* UPortalInteractionComponent::GetPortalGameState() const
{
    return GetWorld()->GetGameState<APortalDefenseGameState>();
}

bool UPortalInteractionComponent::IsInCaptureZone() const
{
    if (APortalDefenseGameMode* GameMode = GetWorld()->GetAuthGameMode<APortalDefenseGameMode>()) {
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) {
            return GameMode->IsPlayerInCaptureZone(OwnerPawn);
        }
    }
    return false;
}

float UPortalInteractionComponent::GetDistanceToPortal() const
{
    if (APortalCore* Portal = Cast<APortalCore>(UGameplayStatics::GetActorOfClass(GetWorld(), APortalCore::StaticClass()))) {
        if (AActor* Owner = GetOwner()) {
            return FVector::Dist(Owner->GetActorLocation(), Portal->GetActorLocation());
        }
    }
    return -1.0f;
}

float UPortalInteractionComponent::GetCaptureZoneRadius() const
{
    if (APortalDefenseGameMode* GameMode = GetWorld()->GetAuthGameMode<APortalDefenseGameMode>()) {
        // Access capture zone radius - would need to expose this in GameMode
        return 500.0f; // Default value, should match GameMode setting
    }
    return 500.0f;
}

void UPortalInteractionComponent::UpdateUI()
{
    if (APortalDefenseGameState* GameState = GetPortalGameState()) {
        UpdateEnergyDisplay(GameState->GetCurrentEnergy());
        UpdateCaptureProgressDisplay(GameState->GetCaptureProgress());
        UpdatePortalHealthDisplay(GameState->GetPortalHealthPercent());
        UpdateGuardCountDisplay(GameState->GetActiveGuardCount());
        UpdateCaptureStatusDisplay(GameState->IsCapturing(), GameState->GetPlayersInZone());
    }
}

void UPortalInteractionComponent::CheckCaptureZoneStatus()
{
    bool bCurrentlyInZone = IsInCaptureZone();

    if (bCurrentlyInZone != bWasInCaptureZone) {
        if (bCurrentlyInZone) {
            EnterCaptureZone();
        } else {
            ExitCaptureZone();
        }
        bWasInCaptureZone = bCurrentlyInZone;
    }

    // Show capture zone indicator when getting close
    float DistanceToPortal = GetDistanceToPortal();
    float CaptureRadius = GetCaptureZoneRadius();

    if (DistanceToPortal > 0 && DistanceToPortal < CaptureRadius * 1.5f) {
        if (!bHasShownCaptureZoneIndicator) {
            ShowCaptureZoneIndicator(true);
            bHasShownCaptureZoneIndicator = true;
        }
    } else if (DistanceToPortal > CaptureRadius * 2.0f) {
        if (bHasShownCaptureZoneIndicator) {
            ShowCaptureZoneIndicator(false);
            bHasShownCaptureZoneIndicator = false;
        }
    }
}

void UPortalInteractionComponent::OnEnergyChanged(int32 NewEnergy)
{
    UpdateEnergyDisplay(NewEnergy);
}

void UPortalInteractionComponent::OnCaptureProgressChanged(float Progress)
{
    UpdateCaptureProgressDisplay(Progress);

    // Show completion message when capture is complete
    if (Progress >= 1.0f) {
        ShowCaptureComplete();
    }
}

void UPortalInteractionComponent::OnPortalHealthChanged(float CurrentHealth, float MaxHealth)
{
    float HealthPercent = MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f;
    UpdatePortalHealthDisplay(HealthPercent);
}

void UPortalInteractionComponent::OnPatrolGuardCountChanged(int32 GuardCount)
{
    UpdateGuardCountDisplay(GuardCount);
}