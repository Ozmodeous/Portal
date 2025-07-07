// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

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
    // Component Configuration for UE 5.5 Optimization
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    bWantsInitializeComponent = true;

    // Overlord Integration Settings
    bAutoRegisterWithOverlord = true;
    bIntegrateWithACFTeams = true;
    bAutoSetPatrolBehavior = true;

    // Portal Defense Configuration
    bDefendPortal = true;
    DefaultGuardTeam = ETeam::ETeam2;

    // Patrol System Defaults
    DefaultPatrolRadius = 400.0f;
    bUseSpawnLocationAsPatrolCenter = true;
    PatrolCenter = FVector::ZeroVector;

    // Detection and Alert System
    PlayerDetectionRange = 1200.0f;
    MaxChaseDistance = 2000.0f;
    bAlertOtherGuards = true;
    AlertRadius = 1500.0f;

    // State Initialization
    bIsPlayerDetected = false;
    bIsInCombat = false;
    DetectedPlayer = nullptr;

    // Performance Settings
    bEnableAdvancedFeatures = true;
    UpdateFrequency = 0.5f;

    // Initialize object references
    ACFAIController = nullptr;
    AssignedPortal = nullptr;
}

void UAIOverseenComponent::BeginPlay()
{
    Super::BeginPlay();

    // Sequential initialization for ACF Ultimate integration
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

    // Bind to owner destruction for cleanup
    if (AActor* Owner = GetOwner()) {
        Owner->OnDestroyed.AddDynamic(this, &UAIOverseenComponent::OnOwnerDestroyed);
    }

    // Initialize ACF-specific components
    InitializeACFComponents();

    UE_LOG(LogTemp, Log, TEXT("AI Overseen Component initialized for %s"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void UAIOverseenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean shutdown sequence for ACF compatibility
    if (ACFAIController) {
        if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
            Overlord->UnregisterAI(ACFAIController);
        }
    }

    // Clear timer handles
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(DetectionUpdateTimer);
        World->GetTimerManager().ClearTimer(PatrolUpdateTimer);
    }

    Super::EndPlay(EndPlayReason);
}

void UAIOverseenComponent::InitializeWithController()
{
    // Locate and validate ACF AI Controller
    if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) {
        ACFAIController = Cast<AACFAIController>(OwnerPawn->GetController());

        if (ACFAIController) {
            // Register with AI Overlord Manager for coordination
            if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
                Overlord->RegisterAI(ACFAIController);
                UE_LOG(LogTemp, Log, TEXT("Registered ACF patrol guard with Overlord: %s"),
                    *ACFAIController->GetName());
            } else {
                UE_LOG(LogTemp, Warning, TEXT("AI Overlord Manager not found for registration"));
            }
        } else {
            UE_LOG(LogTemp, Warning, TEXT("AIOverseenComponent: Owner pawn does not have AACFAIController"));
        }
    } else {
        UE_LOG(LogTemp, Error, TEXT("AIOverseenComponent: Owner is not a Pawn"));
    }
}

void UAIOverseenComponent::ReportDeath()
{
    if (!ACFAIController) {
        return;
    }

    // Report death to Overlord Manager for tactical analysis
    if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
        const FVector DeathLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
        Overlord->RecordAIDeath(ACFAIController, DeathLocation);

        // Trigger area alert if enabled
        if (bAlertOtherGuards) {
            Overlord->AlertNearbyGuards(DeathLocation, AlertRadius);
        }

        UE_LOG(LogTemp, Warning, TEXT("Reported death of %s to Overlord at location %s"),
            *ACFAIController->GetName(), *DeathLocation.ToString());
    }
}

void UAIOverseenComponent::SetCombatTeam(ETeam NewTeam)
{
    // Set team through ACF AI Controller
    if (ACFAIController) {
        ACFAIController->SetCombatTeam(NewTeam);
        UE_LOG(LogTemp, Log, TEXT("Set combat team to %d for patrol guard %s"),
            static_cast<int32>(NewTeam), *GetOwner()->GetName());
    }

    // Also set team via ACF Entity Interface if available
    if (AActor* Owner = GetOwner()) {
        if (Owner->GetClass()->ImplementsInterface(UACFEntityInterface::StaticClass())) {
            IACFEntityInterface::Execute_AssignTeamToEntity(Owner, NewTeam);
        }
    }

    // Update default team setting
    DefaultGuardTeam = NewTeam;
}

bool UAIOverseenComponent::IsACFCharacter() const
{
    // Validate ACF Entity Interface implementation
    return GetOwner() && GetOwner()->GetClass()->ImplementsInterface(UACFEntityInterface::StaticClass());
}

void UAIOverseenComponent::SetOverlordTarget(AActor* Target)
{
    if (!ACFAIController || !Target) {
        UE_LOG(LogTemp, Warning, TEXT("SetOverlordTarget: Invalid ACF controller or target"));
        return;
    }

    // Use ACF's threat management system for target assignment
    if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
        ThreatManager->AddThreat(Target, 100.0f);
    }

    // Set target via ACF targeting component
    if (UATSTargetingComponent* TargetingComp = GetTargetingComponent()) {
        TargetingComp->SetCurrentTarget(Target);
    }

    // Set target in blackboard for behavior tree access
    ACFAIController->SetTargetActorBK(Target);

    // Transition AI to battle state using ACF state system
    ACFAIController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));

    UE_LOG(LogTemp, Log, TEXT("Set overlord target %s for patrol guard %s"),
        *Target->GetName(), *ACFAIController->GetName());
}

void UAIOverseenComponent::ApplyOverlordUpgrade(float MovementMultiplier, float DetectionMultiplier, bool bEnableAdvancedTactics)
{
    if (!ACFAIController) {
        return;
    }

    // Apply upgrades via Portal Defense AI Controller if available
    if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(ACFAIController)) {
        // TODO: Implement upgrade application through Portal Defense AI Controller
        // This would typically involve modifying the AI's data structure with the multipliers
        UE_LOG(LogTemp, Log, TEXT("Applied overlord upgrades to %s: Movement=%.2f, Detection=%.2f, Advanced=%s"),
            *ACFAIController->GetName(), MovementMultiplier, DetectionMultiplier,
            bEnableAdvancedTactics ? TEXT("Yes") : TEXT("No"));
    }

    // Apply detection range upgrade
    PlayerDetectionRange *= DetectionMultiplier;

    // Store advanced tactics setting
    bEnableAdvancedFeatures = bEnableAdvancedTactics;
}

void UAIOverseenComponent::SetPatrolCenter(FVector Center)
{
    PatrolCenter = Center;

    // Update ACF AI Controller with new patrol center if available
    if (ACFAIController) {
        // TODO: Integrate with ACF patrol system
        // This would set the patrol center in the AI's blackboard or behavior tree
        ACFAIController->SetGenericLocationBK(Center);
    }

    UE_LOG(LogTemp, Verbose, TEXT("Set patrol center to %s for %s"),
        *Center.ToString(), *GetOwner()->GetName());
}

void UAIOverseenComponent::SetPatrolRadius(float Radius)
{
    DefaultPatrolRadius = Radius;

    // Update ACF AI Controller with new patrol radius
    if (ACFAIController) {
        // TODO: Set patrol radius in ACF behavior tree or blackboard
        UE_LOG(LogTemp, Verbose, TEXT("Set patrol radius to %.1f for %s"),
            Radius, *GetOwner()->GetName());
    }
}

void UAIOverseenComponent::StartPatrolling()
{
    if (!ACFAIController) {
        return;
    }

    // Set AI to patrol state using ACF state system
    ACFAIController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EPatrol));

    UE_LOG(LogTemp, Log, TEXT("Started patrolling for %s"), *GetOwner()->GetName());
}

void UAIOverseenComponent::StopPatrolling()
{
    if (!ACFAIController) {
        return;
    }

    // Return AI to default state
    ACFAIController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EWait));

    UE_LOG(LogTemp, Log, TEXT("Stopped patrolling for %s"), *GetOwner()->GetName());
}

void UAIOverseenComponent::SetPortalDefenseMode(bool bDefendPortal)
{
    this->bDefendPortal = bDefendPortal;

    if (bDefendPortal) {
        SetPortalAsDefenseTarget();
    } else {
        AssignedPortal = nullptr;
    }
}

void UAIOverseenComponent::AlertToPlayerPresence(FVector PlayerLocation)
{
    if (!ACFAIController) {
        return;
    }

    // Set player location as investigation target
    ACFAIController->SetGenericLocationBK(PlayerLocation);

    // Escalate AI state to investigation
    ACFAIController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EInvestigate));

    // Add threat if player pawn is available
    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
        if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
            ThreatManager->AddThreat(PlayerPawn, 50.0f);
        }
    }

    // Alert overlord system
    if (UAIOverlordManager* Overlord = UAIOverlordManager::GetInstance(GetWorld())) {
        Overlord->RecordPlayerPosition(PlayerLocation);
    }

    UE_LOG(LogTemp, Log, TEXT("Alerted %s to player presence at %s"),
        *GetOwner()->GetName(), *PlayerLocation.ToString());
}

void UAIOverseenComponent::OnPlayerDetected(APawn* PlayerPawn)
{
    if (!PlayerPawn || !ACFAIController) {
        return;
    }

    bIsPlayerDetected = true;
    DetectedPlayer = PlayerPawn;

    // Use ACF threat system for player detection
    if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
        ThreatManager->AddThreat(PlayerPawn, 75.0f);
    }

    // Set as target in ACF targeting system
    if (UATSTargetingComponent* TargetingComp = GetTargetingComponent()) {
        TargetingComp->SetCurrentTarget(PlayerPawn);
    }

    // Transition to combat state
    ACFAIController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));

    // Alert overlord and nearby guards
    if (bAlertOtherGuards) {
        AlertToPlayerPresence(PlayerPawn->GetActorLocation());
    }

    UE_LOG(LogTemp, Warning, TEXT("%s detected player %s"),
        *GetOwner()->GetName(), *PlayerPawn->GetName());
}

void UAIOverseenComponent::OnPlayerLost()
{
    bIsPlayerDetected = false;
    DetectedPlayer = nullptr;

    if (!ACFAIController) {
        return;
    }

    // Clear threats and return to patrol
    if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
        ThreatManager->ClearThreats();
    }

    // Return to patrol state
    ACFAIController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EPatrol));

    UE_LOG(LogTemp, Verbose, TEXT("%s lost player contact"), *GetOwner()->GetName());
}

void UAIOverseenComponent::SetPortalTarget(APortalCore* Portal)
{
    AssignedPortal = Portal;

    if (Portal && ACFAIController) {
        // Set portal as defend target in blackboard
        ACFAIController->SetGenericActorBK(Portal);

        UE_LOG(LogTemp, Log, TEXT("Assigned portal %s to guard %s"),
            *Portal->GetName(), *GetOwner()->GetName());
    }
}

float UAIOverseenComponent::GetDistanceToPortal() const
{
    if (!AssignedPortal || !GetOwner()) {
        return -1.0f;
    }

    return FVector::Dist(GetOwner()->GetActorLocation(), AssignedPortal->GetActorLocation());
}

void UAIOverseenComponent::SetupACFIntegration()
{
    // Configure ACF team integration
    if (bIntegrateWithACFTeams && ACFAIController) {
        SetCombatTeam(DefaultGuardTeam);
    }

    // Initialize ACF component references
    InitializeACFComponents();

    UE_LOG(LogTemp, Verbose, TEXT("Setup ACF integration for %s"), *GetOwner()->GetName());
}

void UAIOverseenComponent::SetupPatrolBehavior()
{
    if (!ACFAIController) {
        return;
    }

    // Set patrol center based on configuration
    if (bUseSpawnLocationAsPatrolCenter && GetOwner()) {
        PatrolCenter = GetOwner()->GetActorLocation();
    }

    // Configure patrol parameters in ACF system
    SetPatrolCenter(PatrolCenter);
    SetPatrolRadius(DefaultPatrolRadius);

    // Start patrolling
    StartPatrolling();

    UE_LOG(LogTemp, Log, TEXT("Setup patrol behavior for %s at center %s with radius %.1f"),
        *GetOwner()->GetName(), *PatrolCenter.ToString(), DefaultPatrolRadius);
}

void UAIOverseenComponent::SetPortalAsDefenseTarget()
{
    // Find nearest portal core for defense assignment
    if (UWorld* World = GetWorld()) {
        APortalCore* NearestPortal = nullptr;
        float NearestDistance = FLT_MAX;

        for (TActorIterator<APortalCore> ActorItr(World); ActorItr; ++ActorItr) {
            APortalCore* Portal = *ActorItr;
            if (Portal && GetOwner()) {
                const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Portal->GetActorLocation());
                if (Distance < NearestDistance) {
                    NearestDistance = Distance;
                    NearestPortal = Portal;
                }
            }
        }

        if (NearestPortal) {
            SetPortalTarget(NearestPortal);
        } else {
            UE_LOG(LogTemp, Warning, TEXT("No portal core found for defense assignment"));
        }
    }
}

void UAIOverseenComponent::OnOwnerDestroyed(AActor* DestroyedActor)
{
    // Report death when owner is destroyed
    ReportDeath();
}

UACFThreatManagerComponent* UAIOverseenComponent::GetThreatManager() const
{
    return ACFAIController ? ACFAIController->FindComponentByClass<UACFThreatManagerComponent>() : nullptr;
}

UATSTargetingComponent* UAIOverseenComponent::GetTargetingComponent() const
{
    return ACFAIController ? ACFAIController->FindComponentByClass<UATSTargetingComponent>() : nullptr;
}

void UAIOverseenComponent::UpdateDetectionState()
{
    // Periodic update for player detection and state management
    if (!ACFAIController || !bEnableAdvancedFeatures) {
        return;
    }

    // Check for player proximity
    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
        const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), PlayerPawn->GetActorLocation());

        if (Distance <= PlayerDetectionRange && !bIsPlayerDetected) {
            OnPlayerDetected(PlayerPawn);
        } else if (Distance > MaxChaseDistance && bIsPlayerDetected) {
            OnPlayerLost();
        }
    }
}

void UAIOverseenComponent::HandlePlayerProximity()
{
    // Advanced player proximity handling for tactical AI behavior
    if (!bIsPlayerDetected || !DetectedPlayer || !ACFAIController) {
        return;
    }

    const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), DetectedPlayer->GetActorLocation());

    // Escalate threat based on proximity
    if (UACFThreatManagerComponent* ThreatManager = GetThreatManager()) {
        const float ThreatLevel = FMath::Clamp(200.0f - (Distance / 10.0f), 50.0f, 200.0f);
        ThreatManager->AddThreat(DetectedPlayer, ThreatLevel);
    }
}

void UAIOverseenComponent::InitializeACFComponents()
{
    // Cache ACF component references for performance optimization
    if (!ACFAIController) {
        return;
    }

    // Validate essential ACF components
    UACFThreatManagerComponent* ThreatManager = GetThreatManager();
    UATSTargetingComponent* TargetingComp = GetTargetingComponent();

    if (!ThreatManager) {
        UE_LOG(LogTemp, Warning, TEXT("ACF Threat Manager Component not found on %s"),
            *ACFAIController->GetName());
    }

    if (!TargetingComp) {
        UE_LOG(LogTemp, Warning, TEXT("ACF Targeting Component not found on %s"),
            *ACFAIController->GetName());
    }

    // Setup periodic update timers if advanced features are enabled
    if (bEnableAdvancedFeatures && GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(DetectionUpdateTimer, this,
            &UAIOverseenComponent::UpdateDetectionState, UpdateFrequency, true);

        GetWorld()->GetTimerManager().SetTimer(PatrolUpdateTimer, this,
            &UAIOverseenComponent::HandlePlayerProximity, UpdateFrequency * 0.5f, true);
    }
}