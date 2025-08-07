#include "PortalCore.h"
#include "Components/ACFTeamManagerComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollection.h"
#include "Net/UnrealNetwork.h"


APortalCore::APortalCore()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    // Create Root Component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Create Portal Mesh
    PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
    PortalMesh->SetupAttachment(RootComponent);
    PortalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PortalMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    PortalMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    PortalMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

    // Create Interaction Sphere
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(300.0f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    InteractionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

    // Create Health Widget
    HealthWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidget"));
    HealthWidget->SetupAttachment(RootComponent);
    HealthWidget->SetWidgetSpace(EWidgetSpace::Screen);
    HealthWidget->SetDrawSize(FVector2D(200.0f, 50.0f));
    HealthWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));

    // Create Team Manager
    TeamManager = CreateDefaultSubobject<UACFTeamManagerComponent>(TEXT("TeamManager"));

    // Health Properties
    MaxHealth = 1000.0f;
    CurrentHealth = MaxHealth;
    bInvulnerable = false;
    bIsDestroyed = false;

    // Energy Properties
    EnergyCapacity = 100;
    EnergyEfficiency = 1.0f;
    BaseEnergyExtraction = 50;

    // Interaction Properties
    bCanInteract = true;
    InteractionRange = 300.0f;
    InteractableName = FText::FromString("Portal");

    // Visual Properties
    HealthyColor = FLinearColor::Green;
    DamagedColor = FLinearColor::Yellow;
    CriticalColor = FLinearColor::Red;
}

void APortalCore::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
    UpdateVisualState();
}

void APortalCore::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateVisualState();
}

// ACF Interactable Interface Implementation
void APortalCore::OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType)
{
    PlayerInteract(Pawn);
}

void APortalCore::OnLocalInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType)
{
    PlayerInteract(Pawn);
}

void APortalCore::OnInteractableRegisteredByPawn_Implementation(APawn* Pawn)
{
    UE_LOG(LogTemp, Log, TEXT("Portal registered by %s"), *Pawn->GetName());
}

void APortalCore::OnInteractableUnregisteredByPawn_Implementation(APawn* Pawn)
{
    UE_LOG(LogTemp, Log, TEXT("Portal unregistered by %s"), *Pawn->GetName());
}

FText APortalCore::GetInteractableName_Implementation()
{
    return InteractableName;
}

bool APortalCore::CanBeInteracted_Implementation(APawn* Pawn)
{
    return CanInteract();
}

float APortalCore::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    ApplyPortalDamage(ActualDamage, DamageCauser);
    return ActualDamage;
}

void APortalCore::ApplyPortalDamage(float DamageAmount, AActor* DamageCauser)
{
    if (!HasAuthority() || bInvulnerable || bIsDestroyed || DamageAmount <= 0.0f) {
        return;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
    PlayDamageEffect();

    if (CurrentHealth <= 0.0f && !bIsDestroyed) {
        HandleDestruction();
    }
}

void APortalCore::RestoreHealth(float HealAmount)
{
    if (!HasAuthority() || bIsDestroyed || HealAmount <= 0.0f) {
        return;
    }

    CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
}

int32 APortalCore::ExtractEnergy()
{
    if (bIsDestroyed) {
        return 0;
    }

    int32 ExtractedAmount = FMath::RoundToInt(BaseEnergyExtraction * EnergyEfficiency * GetHealthPercent());
    PlayEnergyExtractionEffect();
    OnEnergyExtracted.Broadcast(ExtractedAmount);

    return ExtractedAmount;
}

void APortalCore::PlayerInteract(APawn* InteractingPlayer)
{
    if (!CanInteract() || !InteractingPlayer) {
        return;
    }

    int32 ExtractedEnergy = ExtractEnergy();
    UE_LOG(LogTemp, Log, TEXT("Player %s interacted with Portal, extracted %d energy"),
        *InteractingPlayer->GetName(), ExtractedEnergy);
}

void APortalCore::UpdateVisualState()
{
    if (!PortalMesh) {
        return;
    }

    FLinearColor CurrentColor = GetHealthBasedColor();

    if (UMaterialInstanceDynamic* DynamicMaterial = PortalMesh->CreateAndSetMaterialInstanceDynamic(0)) {
        DynamicMaterial->SetVectorParameterValue(TEXT("HealthColor"), CurrentColor);
        DynamicMaterial->SetScalarParameterValue(TEXT("HealthPercent"), GetHealthPercent());
    }
}

void APortalCore::HandleDestruction()
{
    if (bIsDestroyed) {
        return;
    }

    bIsDestroyed = true;
    bCanInteract = false;

    PlayDestructionEffect();
    OnPortalDestroyed.Broadcast();

    PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    UE_LOG(LogTemp, Warning, TEXT("Portal Core has been destroyed!"));
}

FLinearColor APortalCore::GetHealthBasedColor() const
{
    float HealthPercent = GetHealthPercent();

    if (HealthPercent > 0.6f) {
        return HealthyColor;
    } else if (HealthPercent > 0.2f) {
        return DamagedColor;
    } else {
        return CriticalColor;
    }
}

void APortalCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APortalCore, MaxHealth);
    DOREPLIFETIME(APortalCore, CurrentHealth);
}