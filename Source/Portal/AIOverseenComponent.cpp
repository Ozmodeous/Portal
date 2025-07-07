// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AIOverseenComponent.h"
#include "Components/CombatBehaviourComponent.h"
#include "EliteAIIntelligenceComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "PortalCore.h"
#include "PortalDefenseAIController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAIOverseenComponent, Log, All);

UAIOverseenComponent::UAIOverseenComponent()
{
    // Set this component to be ticked every frame for real-time coordination
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // Initialize default coordination settings
    TacticalUpdateRate = 2.0f;
    MaxManagedAI = 10;
    bEnableEliteCoordination = true;
    bDefendPortal = true;
    ThreatEscalationThreshold = 3;
    CoordinationRadius = 1500.0f;
    MaxDefensiveRadius = 800.0f;

    // Initialize internal state
    CurrentThreatLevel = EThreatLevel::Low;
    DefensivePosition = FVector::ZeroVector;
}

void UAIOverseenComponent::BeginPlay()
{
    Super::BeginPlay();

    // Initialize the tactical coordination system
    InitializeTacticalSystem();

    // Find the Portal Core for defense coordination
    PortalCore = FindPortalCore();

    // Start tactical update timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(
            TacticalUpdateTimer,
            this,
            &UAIOverseenComponent::UpdateTacticalCoordination,
            1.0f / TacticalUpdateRate,
            true);
    }

    UE_LOG(LogAIOverseenComponent, Log, TEXT("AI Overseen Component initialized - Portal Core: %s"),
        IsValid(PortalCore) ? *PortalCore->GetName() : TEXT("Not Found"));
}

void UAIOverseenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear the tactical update timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(TacticalUpdateTimer);
    }

    // Clear all managed AI controllers
    ManagedAIControllers.Empty();

    UE_LOG(LogAIOverseenComponent, Log, TEXT("AI Overseen Component cleanup completed"));

    Super::EndPlay(EndPlayReason);
}

void UAIOverseenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Periodically clean up invalid AI controller references
    static float CleanupTimer = 0.0f;
    CleanupTimer += DeltaTime;
    if (CleanupTimer >= 3.0f) // Clean up every 3 seconds
    {
        ManagedAIControllers.RemoveAll([](const TObjectPtr<APortalDefenseAIController>& Controller) {
            return !IsValid(Controller);
        });
        CleanupTimer = 0.0f;
    }

    // Update threat assessment
    UpdateThreatAssessment();
}

void UAIOverseenComponent::InitializeTacticalSystem()
{
    // Register with existing AI controllers in the world
    if (UWorld* World = GetWorld()) {
        for (TActorIterator<APortalDefenseAIController> ActorItr(World); ActorItr; ++ActorItr) {
            APortalDefenseAIController* AIController = *ActorItr;
            if (IsAIControllerEligible(AIController)) {
                RegisterManagedAI(AIController);
            }
        }
    }

    // Set initial defensive positioning if portal defense is enabled
    if (bDefendPortal && IsValid(PortalCore)) {
        DefensivePosition = PortalCore->GetActorLocation();
    }

    UE_LOG(LogAIOverseenComponent, Log, TEXT("Tactical system initialized with %d managed AI controllers"), ManagedAIControllers.Num());
}

void UAIOverseenComponent::RegisterManagedAI(APortalDefenseAIController* AIController)
{
    if (!IsValid(AIController)) {
        UE_LOG(LogAIOverseenComponent, Warning, TEXT("Attempted to register invalid AI controller"));
        return;
    }

    // Check if we're at capacity
    if (ManagedAIControllers.Num() >= MaxManagedAI) {
        UE_LOG(LogAIOverseenComponent, Warning, TEXT("Cannot register AI controller - at maximum capacity (%d)"), MaxManagedAI);
        return;
    }

    // Check if already registered
    if (ManagedAIControllers.Contains(AIController)) {
        UE_LOG(LogAIOverseenComponent, VeryVerbose, TEXT("AI controller %s is already registered"), *AIController->GetName());
        return;
    }

    // Verify eligibility
    if (!IsAIControllerEligible(AIController)) {
        UE_LOG(LogAIOverseenComponent, VeryVerbose, TEXT("AI controller %s is not eligible for coordination"), *AIController->GetName());
        return;
    }

    // Add to managed controllers
    ManagedAIControllers.Add(AIController);

    // Configure for tactical coordination
    ConfigureAIForTacticalCoordination(AIController);

    UE_LOG(LogAIOverseenComponent, VeryVerbose, TEXT("Registered AI controller %s for tactical coordination"), *AIController->GetName());
}

void UAIOverseenComponent::UnregisterManagedAI(APortalDefenseAIController* AIController)
{
    if (ManagedAIControllers.Remove(AIController) > 0) {
        UE_LOG(LogAIOverseenComponent, VeryVerbose, TEXT("Unregistered AI controller %s from tactical coordination"),
            IsValid(AIController) ? *AIController->GetName() : TEXT("Invalid"));
    }
}

void UAIOverseenComponent::UpdateTacticalCoordination()
{
    if (ManagedAIControllers.Num() == 0) {
        return;
    }

    // Calculate defensive positions if defending portal
    if (bDefendPortal) {
        CalculateDefensivePositions();
    }

    // Apply coordination commands to managed AI
    ApplyCoordinationCommands();

    // Update threat assessment
    UpdateThreatAssessment();

    UE_LOG(LogAIOverseenComponent, VeryVerbose, TEXT("Updated tactical coordination for %d AI controllers"), ManagedAIControllers.Num());
}

void UAIOverseenComponent::SetThreatLevel(EThreatLevel NewThreatLevel)
{
    if (CurrentThreatLevel != NewThreatLevel) {
        EThreatLevel OldThreatLevel = CurrentThreatLevel;
        CurrentThreatLevel = NewThreatLevel;

        UE_LOG(LogAIOverseenComponent, Log, TEXT("Threat level changed from %d to %d"),
            static_cast<int32>(OldThreatLevel), static_cast<int32>(NewThreatLevel));

        // Immediately update coordination based on new threat level
        UpdateTacticalCoordination();
    }
}

APortalCore* UAIOverseenComponent::FindPortalCore()
{
    if (UWorld* World = GetWorld()) {
        for (TActorIterator<APortalCore> ActorItr(World); ActorItr; ++ActorItr) {
            APortalCore* Core = *ActorItr;
            if (IsValid(Core)) {
                return Core;
            }
        }
    }
    return nullptr;
}

bool UAIOverseenComponent::IsAIControllerEligible(APortalDefenseAIController* AIController) const
{
    if (!IsValid(AIController)) {
        return false;
    }

    // Check if AI controller has a valid pawn
    if (!IsValid(AIController->GetPawn())) {
        return false;
    }

    // Check if within coordination radius
    if (IsValid(PortalCore)) {
        float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), PortalCore->GetActorLocation());
        if (Distance > CoordinationRadius) {
            return false;
        }
    }

    return true;
}

void UAIOverseenComponent::ConfigureAIForTacticalCoordination(APortalDefenseAIController* AIController)
{
    if (!IsValid(AIController)) {
        return;
    }

    // Enable elite coordination if available and configured
    if (bEnableEliteCoordination) {
        if (UEliteAIIntelligenceComponent* EliteComponent = AIController->FindComponentByClass<UEliteAIIntelligenceComponent>()) {
            // Configure elite AI for tactical coordination
            EliteComponent->SetTacticalCoordinationEnabled(true);
            EliteComponent->SetCoordinationRadius(CoordinationRadius);
        }
    }

    UE_LOG(LogAIOverseenComponent, VeryVerbose, TEXT("Configured AI controller %s for tactical coordination"), *AIController->GetName());
}

void UAIOverseenComponent::UpdateThreatAssessment()
{
    if (!IsValid(PortalCore)) {
        return;
    }

    // Count nearby enemies and assess threat level
    int32 NearbyEnemyCount = 0;

    if (UWorld* World = GetWorld()) {
        // Find player controllers near the portal
        for (TActorIterator<APlayerController> ActorItr(World); ActorItr; ++ActorItr) {
            APlayerController* PC = *ActorItr;
            if (IsValid(PC) && IsValid(PC->GetPawn())) {
                float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), PortalCore->GetActorLocation());
                if (Distance <= CoordinationRadius) {
                    NearbyEnemyCount++;
                }
            }
        }
    }

    // Determine threat level based on enemy count and proximity
    EThreatLevel NewThreatLevel = EThreatLevel::Low;

    if (NearbyEnemyCount >= ThreatEscalationThreshold * 3) {
        NewThreatLevel = EThreatLevel::Extreme;
    } else if (NearbyEnemyCount >= ThreatEscalationThreshold * 2) {
        NewThreatLevel = EThreatLevel::Critical;
    } else if (NearbyEnemyCount >= ThreatEscalationThreshold) {
        NewThreatLevel = EThreatLevel::High;
    } else if (NearbyEnemyCount > 0) {
        NewThreatLevel = EThreatLevel::Medium;
    }

    // Update threat level if changed
    if (NewThreatLevel != CurrentThreatLevel) {
        SetThreatLevel(NewThreatLevel);
    }
}

void UAIOverseenComponent::CalculateDefensivePositions()
{
    if (!IsValid(PortalCore) || ManagedAIControllers.Num() == 0) {
        return;
    }

    FVector PortalLocation = PortalCore->GetActorLocation();
    int32 AICount = ManagedAIControllers.Num();

    // Calculate defensive positions in a circle around the portal
    float AngleStep = 360.0f / FMath::Max(1, AICount);
    float DefensiveRadius = FMath::Min(MaxDefensiveRadius, CoordinationRadius * 0.5f);

    for (int32 i = 0; i < ManagedAIControllers.Num(); ++i) {
        if (IsValid(ManagedAIControllers[i])) {
            float Angle = AngleStep * i;
            FVector DefensivePos = PortalLocation + FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * DefensiveRadius, FMath::Sin(FMath::DegreesToRadians(Angle)) * DefensiveRadius, 0.0f);

            // Set defensive position for the AI controller
            ManagedAIControllers[i]->SetDefensivePosition(DefensivePos);
        }
    }
}

void UAIOverseenComponent::ApplyCoordinationCommands()
{
    for (TObjectPtr<APortalDefenseAIController> AIController : ManagedAIControllers) {
        if (IsValid(AIController)) {
            // Apply threat-based behavior changes
            switch (CurrentThreatLevel) {
            case EThreatLevel::Low:
                AIController->SetAIBehaviorMode(EAIBehaviorMode::Patrol);
                break;

            case EThreatLevel::Medium:
                AIController->SetAIBehaviorMode(EAIBehaviorMode::Alert);
                break;

            case EThreatLevel::High:
            case EThreatLevel::Critical:
            case EThreatLevel::Extreme:
                AIController->SetAIBehaviorMode(EAIBehaviorMode::Combat);
                break;
            }

            // Update coordination parameters
            AIController->SetCoordinationEnabled(true);
            AIController->SetThreatLevel(CurrentThreatLevel);
        }
    }
}

// Blueprint Accessible Getters
int32 UAIOverseenComponent::GetManagedAICount() const
{
    return ManagedAIControllers.Num();
}

EThreatLevel UAIOverseenComponent::GetCurrentThreatLevel() const
{
    return CurrentThreatLevel;
}

bool UAIOverseenComponent::IsCoordinationActive() const
{
    return TacticalUpdateTimer.IsValid() && ManagedAIControllers.Num() > 0;
}

TArray<APortalDefenseAIController*> UAIOverseenComponent::GetManagedAIControllers() const
{
    TArray<APortalDefenseAIController*> Result;
    for (const TObjectPtr<APortalDefenseAIController>& Controller : ManagedAIControllers) {
        if (IsValid(Controller)) {
            Result.Add(Controller);
        }
    }
    return Result;
}