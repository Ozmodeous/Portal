#include "AIOverseenComponent.h"
#include "AIOverlordManager.h"
#include "ARSStatisticsComponent.h"
#include "ATSTargetingComponent.h"
#include "Components/ACFCommandsManagerComponent.h"
#include "Components/ACFDamageHandlerComponent.h"
#include "Components/ACFTeamManagerComponent.h"
#include "Components/ACFThreatManagerComponent.h"
#include "Game/ACFFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ACFEntityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "PortalCore.h"
#include "PortalDefenseAIController.h"

UAIOverseenComponent::UAIOverseenComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoRegisterWithOverlord = true;
    bIntegrateWithACFTeams = true;
    bAutoSetPatrolBehavior = true;
    bDefendPortal = true;
    DefaultGuardTeam = ETeam::ETeam2;
    DefaultPatrolRadius = 400.0f;
    bUseSpawnLocationAsPatrolCenter = true;
    PlayerDetectionRange = 1200.0f;
    MaxChaseDistance = 2000.0f;
    bAlertOtherGuards = true;
    AlertRadius = 1500.0f;
}

void UAIOverseenComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRegisterWithOverlord) {
        InitializeWithController();
    }

    if (bIntegrateWithACFTeams) {
        SetupACFIntegration();
    }

    if (bAutoSetPatrolBehavior) {
        SetupPatrolBehavior();
    }

    if (bDefendPortal) {
        SetPortalAsDefenseTarget();
    }

    // Bind to owner destruction
    if (AActor* Owner = GetOwner()) {
        Owner->OnDestroyed.AddDynamic(this, &UAIOverseenComponent::OnOwnerDestroyed);
    }
}

void UAIOverseenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ACFAIController) {
        if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
            Overlord->UnregisterAI(ACFAIController);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UAIOverseenComponent::InitializeWithController()
{
    if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) {
        ACFAIController = Cast<AACFAIController>(OwnerPawn->GetController());

        if (ACFAIController) {
            // Register with overlord
            if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
                Overlord->RegisterAI(ACFAIController);
                UE_LOG(LogTemp, Log, TEXT("Registered patrol guard with Overlord: %s"), *ACFAIController->GetName());
            }
        } else {
            UE_LOG(LogTemp, Warning, TEXT("AIOverseenComponent: Owner pawn does not have AACFAIController"));
        }
    }
}

void UAIOverseenComponent::ReportDeath()
{
    if (ACFAIController) {
        if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
            FVector DeathLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
            Overlord->RecordAIDeath(ACFAIController, DeathLocation);

            // Alert nearby guards if enabled
            if (bAlertOtherGuards) {
                Overlord->AlertNearbyGuards(DeathLocation, AlertRadius);
            }
        }
    }
}

void UAIOverseenComponent::SetCombatTeam(ETeam NewTeam)
{
    if (ACFAIController) {
        ACFAIController->SetCombatTeam(NewTeam);
        UE_LOG(LogTemp, Log, TEXT("Set combat team to %d for patrol guard %s"), (int32)NewTeam, *GetOwner()->GetName());
    }

    // Also set team via Entity interface if available
    if (AActor* Owner = GetOwner()) {
        if (Owner->GetClass()->ImplementsInterface(UACFEntityInterface::StaticClass())) {
            IACFEntityInterface::Execute_AssignTeamToEntity(Owner, NewTeam);
        }
    }
}

bool UAIOverseenComponent::IsACFCharacter() const
{
    return GetOwner() && GetOwner()->GetClass()->ImplementsInterface(UACFEntityInterface::StaticClass());
}

void UAIOverseenComponent::SetOverlordTarget(AActor* Target)
{
    if (!ACFAIController || !Target)
        return;

    // Use ACF's threat system
    if (UACFThreatManagerComponent* ThreatManager = ACFAIController->GetThreatManager()) {
        ThreatManager->AddThreat(Target, 100.0f);
    }

    // Set target via ACF targeting component
    if (UATSTargetingComponent* TargetingComp = ACFAIController->FindComponentByClass<UATSTargetingComponent>()) {
        TargetingComp->SetCurrentTarget(Target);
    }

    // Set target in blackboard
    ACFAIController->SetTargetActorBK(Target);

    // Set AI to battle state
    ACFAIController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));

    UE_LOG(LogTemp, Log, TEXT("Set overlord target %s for patrol guard %s"), *Target->GetName(), *ACFAIController->GetName());
}

void UAIOverseenComponent::ApplyOverlordUpgrade(float MovementMultiplier, float DetectionMultiplier, bool bEnableAdvancedTactics)
{
    if (!ACFAIController)
        return;

    // Apply upgrades via Portal Defense AI Controller
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
        FPortalAIData CurrentData = PatrolAI->GetCurrentAIData();

        // Apply multipliers
        CurrentData.MovementSpeed *= MovementMultiplier;
        CurrentData.PlayerDetectionRange *= DetectionMultiplier;
        CurrentData.bUseAdvancedPathfinding = bEnableAdvancedTactics;
        CurrentData.bCanFlank = bEnableAdvancedTactics;

        PatrolAI->ApplyAIUpgrade(CurrentData);
    }

    // Apply movement speed upgrade via ACF's statistics system
    if (AActor* Owner = GetOwner()) {
        if (UARSStatisticsComponent* StatsComp = Owner->FindComponentByClass<UARSStatisticsComponent>()) {
            FAttributesSetModifier Modifier;
            Modifier.Guid = FGuid::NewGuid();

            // Movement speed modifier
            if (MovementMultiplier != 1.0f) {
                FAttributeModifier SpeedMod;
                SpeedMod.AttributeType = FGameplayTag::RequestGameplayTag(FName("RPG.Parameters.MovementSpeed"));
                SpeedMod.ModType = EModifierType::EPercentage;
                SpeedMod.Value = (MovementMultiplier - 1.0f) * 100.0f;
                Modifier.AttributesMod.Add(SpeedMod);
            }

            if (Modifier.AttributesMod.Num() > 0) {
                StatsComp->AddAttributeSetModifier(Modifier);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Applied overlord upgrade to patrol guard %s - Speed: %.2f, Detection: %.2f, Tactics: %s"),
        *ACFAIController->GetName(), MovementMultiplier, DetectionMultiplier,
        bEnableAdvancedTactics ? TEXT("Enabled") : TEXT("Disabled"));
}

void UAIOverseenComponent::SetPatrolCenter(FVector Center)
{
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
        PatrolAI->SetPatrolCenter(Center);
        UE_LOG(LogTemp, Log, TEXT("Set patrol center for %s to %s"), *GetOwner()->GetName(), *Center.ToString());
    }
}

void UAIOverseenComponent::SetPatrolRadius(float Radius)
{
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
        PatrolAI->SetPatrolRadius(Radius);
        UE_LOG(LogTemp, Log, TEXT("Set patrol radius for %s to %.1f"), *GetOwner()->GetName(), Radius);
    }
}

void UAIOverseenComponent::StartPatrolling()
{
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
        PatrolAI->StartPatrolling();
        UE_LOG(LogTemp, Log, TEXT("Started patrolling for %s"), *GetOwner()->GetName());
    }
}

void UAIOverseenComponent::StopPatrolling()
{
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
        PatrolAI->StopPatrolling();
        UE_LOG(LogTemp, Log, TEXT("Stopped patrolling for %s"), *GetOwner()->GetName());
    }
}

void UAIOverseenComponent::SetPortalDefenseMode(bool bDefendPortalValue)
{
    bDefendPortal = bDefendPortalValue;

    if (bDefendPortal) {
        SetPortalAsDefenseTarget();
    }
}

void UAIOverseenComponent::AlertToPlayerPresence(FVector PlayerLocation)
{
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
        TArray<FVector> AlertParams = { PlayerLocation };
        PatrolAI->ReceiveOverlordCommand("InvestigateAlert", AlertParams);

        // Alert overlord about player presence
        if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
            Overlord->RecordPlayerIncursion(PlayerLocation);
        }
    }
}

void UAIOverseenComponent::OnOwnerDestroyed(AActor* DestroyedActor)
{
    ReportDeath();
}

void UAIOverseenComponent::OnACFCharacterDeath()
{
    ReportDeath();
}

void UAIOverseenComponent::SetupACFIntegration()
{
    if (AActor* Owner = GetOwner()) {
        // Set default team for portal defense
        SetCombatTeam(DefaultGuardTeam);

        // Bind to ACF damage handler for death detection
        if (UACFDamageHandlerComponent* DamageHandler = Owner->FindComponentByClass<UACFDamageHandlerComponent>()) {
            if (!DamageHandler->OnOwnerDeath.IsAlreadyBound(this, &UAIOverseenComponent::OnACFCharacterDeath)) {
                DamageHandler->OnOwnerDeath.AddDynamic(this, &UAIOverseenComponent::OnACFCharacterDeath);
            }
        }
    }
}

void UAIOverseenComponent::SetPortalAsDefenseTarget()
{
    // Find portal in level and set as defense objective
    if (APortalCore* Portal = Cast<APortalCore>(UGameplayStatics::GetActorOfClass(GetWorld(), APortalCore::StaticClass()))) {
        // Delay setting target to ensure controller is properly initialized
        FTimerHandle DelayHandle;
        GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this, Portal]() {
            if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
                PatrolAI->SetPortalTarget(Portal);
            } }, 0.5f, false);
    }
}

void UAIOverseenComponent::SetupPatrolBehavior()
{
    // Delay patrol setup to ensure controller is properly initialized
    FTimerHandle DelayHandle;
    GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this]() {
        if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
            // Set patrol center
            FVector PatrolCenter;
            if (bUseSpawnLocationAsPatrolCenter && GetOwner()) {
                PatrolCenter = GetOwner()->GetActorLocation();
            } else {
                PatrolCenter = CustomPatrolCenter;
            }
            
            SetPatrolCenter(PatrolCenter);
            SetPatrolRadius(DefaultPatrolRadius);
            StartPatrolling();
            
            UE_LOG(LogTemp, Log, TEXT("Setup patrol behavior for %s at center %s with radius %.1f"), 
                *GetOwner()->GetName(), *PatrolCenter.ToString(), DefaultPatrolRadius);
        } }, 1.0f, false);
}