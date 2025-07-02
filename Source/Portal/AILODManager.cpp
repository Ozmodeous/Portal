// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AILODManager.h"
#include "Algo/Sort.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"

TObjectPtr<UAILODManager> UAILODManager::InstancePtr = nullptr;

UAILODManager::UAILODManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;

    // Default LOD settings optimized for 200 AI
    LODSettings.InactiveDistance = 5000.0f;
    LODSettings.MinimalDistance = 3500.0f;
    LODSettings.StandardDistance = 2000.0f;
    LODSettings.HighDistance = 1000.0f;
    LODSettings.MaximumDistance = 500.0f;
    LODSettings.MaxHighLODAI = 15;
    LODSettings.MaxMaximumLODAI = 8;
    LODSettings.LODUpdateFrequency = 0.5f;
    LODSettings.bUsePlayerPredictiveLOD = true;
    LODSettings.PredictionRadius = 1500.0f;

    AverageFrameTime = 16.67f;
    PredictedPlayerPosition = FVector::ZeroVector;
    LastPlayerPosition = FVector::ZeroVector;
}

void UAILODManager::BeginPlay()
{
    Super::BeginPlay();

    InstancePtr = this;

    UpdatePlayerReference();
    StartLODUpdateTimer();

    // Start performance monitoring
    GetWorld()->GetTimerManager().SetTimer(PerformanceMonitorTimer, this,
        &UAILODManager::MonitorPerformance, 1.0f, true);

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager initialized - Target: 200+ AI support"));
}

void UAILODManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopLODUpdateTimer();

    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(PerformanceMonitorTimer);
    }

    if (InstancePtr == this) {
        InstancePtr = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

UAILODManager* UAILODManager::GetInstance(UWorld* World)
{
    if (!InstancePtr && World) {
        // Try to find existing instance
        for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
            if (UAILODManager* LODManager = ActorItr->FindComponentByClass<UAILODManager>()) {
                InstancePtr = LODManager;
                break;
            }
        }

        // Create new instance if none found
        if (!InstancePtr) {
            if (AGameModeBase* GameMode = World->GetAuthGameMode()) {
                InstancePtr = NewObject<UAILODManager>(GameMode);
                GameMode->AddInstanceComponent(InstancePtr);
                InstancePtr->RegisterComponent();
            }
        }
    }

    return InstancePtr;
}

void UAILODManager::RegisterAI(AACFAIController* AIController)
{
    if (!AIController || !IsValid(AIController)) {
        return;
    }

    // Check if already registered
    for (const FAILODData& Data : RegisteredAI) {
        if (Data.AIController == AIController) {
            return;
        }
    }

    FAILODData NewAIData;
    NewAIData.AIController = AIController;
    NewAIData.CurrentLODLevel = EAILODLevel::Standard;
    NewAIData.DistanceToPlayer = 9999.0f;
    NewAIData.LODPriority = 1.0f;
    NewAIData.LastLODUpdateTime = GetWorld()->GetTimeSeconds();

    RegisteredAI.Add(NewAIData);

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Registered AI %s (Total: %d)"),
        *AIController->GetName(), RegisteredAI.Num());
}

void UAILODManager::UnregisterAI(AACFAIController* AIController)
{
    if (!AIController) {
        return;
    }

    RegisteredAI.RemoveAll([AIController](const FAILODData& Data) {
        return Data.AIController == AIController;
    });

    UE_LOG(LogTemp, Log, TEXT("AI LOD Manager: Unregistered AI %s (Total: %d)"),
        *AIController->GetName(), RegisteredAI.Num());
}

void UAILODManager::UpdateAILOD()
{
    if (RegisteredAI.Num() == 0) {
        return;
    }

    CleanupInvalidAI();
    UpdatePlayerReference();

    if (!CachedPlayerPawn) {
        return;
    }

    if (LODSettings.bUsePlayerPredictiveLOD) {
        PredictPlayerMovement();
    }

    const FVector PlayerPosition = CachedPlayerPawn->GetActorLocation();
    const FVector TargetPosition = LODSettings.bUsePlayerPredictiveLOD ? PredictedPlayerPosition : PlayerPosition;
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // Update distances and priorities
    for (FAILODData& AIData : RegisteredAI) {
        if (!AIData.AIController || !IsValid(AIData.AIController)) {
            continue;
        }

        if (APawn* AIPawn = AIData.AIController->GetPawn()) {
            AIData.DistanceToPlayer = FVector::Dist(AIPawn->GetActorLocation(), TargetPosition);
            AIData.LODPriority = CalculateAIPriority(AIData);
            AIData.LastLODUpdateTime = CurrentTime;

            // Update combat status
            if (APortalDefenseAIController* PatrolAI = Cast<APortalDefenseAIController>(AIData.AIController)) {
                AIData.bInCombat = PatrolAI->IsInCombat();
                AIData.bIsEngagingPlayer = PatrolAI->IsEngagingPlayer();
            }
        }
    }

    CalculateLODPriorities();
    ApplyLODLimits();
}

void UAILODManager::SetAILODLevel(AACFAIController* AIController, EAILODLevel NewLODLevel)
{
    if (!AIController) {
        return;
    }

    for (FAILODData& AIData : RegisteredAI) {
        if (AIData.AIController == AIController) {
            SetAILODInternal(AIData, NewLODLevel);
            break;
        }
    }
}

void UAILODManager::ForceHighLOD(AACFAIController* AIController, float Duration)
{
    SetAILODLevel(AIController, EAILODLevel::High);

    // Set timer to revert LOD after duration
    FTimerHandle RevertTimer;
    GetWorld()->GetTimerManager().SetTimer(RevertTimer, [this, AIController]() {
        for (FAILODData& AIData : RegisteredAI)
        {
            if (AIData.AIController == AIController)
            {
                EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);
                SetAILODInternal(AIData, OptimalLOD);
                break;
            }
        } }, Duration, false);
}

void UAILODManager::ForceMaximumLOD(AACFAIController* AIController, float Duration)
{
    SetAILODLevel(AIController, EAILODLevel::Maximum);

    // Set timer to revert LOD after duration
    FTimerHandle RevertTimer;
    GetWorld()->GetTimerManager().SetTimer(RevertTimer, [this, AIController]() {
        for (FAILODData& AIData : RegisteredAI)
        {
            if (AIData.AIController == AIController)
            {
                EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);
                SetAILODInternal(AIData, OptimalLOD);
                break;
            }
        } }, Duration, false);
}

int32 UAILODManager::GetAICountByLOD(EAILODLevel LODLevel) const
{
    return RegisteredAI.FilterByPredicate([LODLevel](const FAILODData& Data) {
                           return Data.CurrentLODLevel == LODLevel;
                       })
        .Num();
}

void UAILODManager::UpdatePlayerReference()
{
    if (!CachedPlayerPawn || !IsValid(CachedPlayerPawn)) {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0)) {
            CachedPlayerPawn = PC->GetPawn();
        }
    }
}

void UAILODManager::CalculateLODPriorities()
{
    // Sort AI by priority (highest first)
    RegisteredAI.Sort([](const FAILODData& A, const FAILODData& B) {
        return A.LODPriority > B.LODPriority;
    });

    // Apply optimal LOD based on priority and distance
    for (FAILODData& AIData : RegisteredAI) {
        EAILODLevel OptimalLOD = CalculateOptimalLOD(AIData);

        if (AIData.CurrentLODLevel != OptimalLOD) {
            SetAILODInternal(AIData, OptimalLOD);
        }
    }
}

void UAILODManager::ApplyLODLimits()
{
    int32 HighLODCount = 0;
    int32 MaximumLODCount = 0;

    // First pass: Count current high-level LODs
    for (const FAILODData& AIData : RegisteredAI) {
        if (AIData.CurrentLODLevel == EAILODLevel::High) {
            HighLODCount++;
        } else if (AIData.CurrentLODLevel == EAILODLevel::Maximum) {
            MaximumLODCount++;
        }
    }

    // Second pass: Downgrade excess high-level LODs
    for (FAILODData& AIData : RegisteredAI) {
        // Downgrade excess Maximum LOD AI
        if (AIData.CurrentLODLevel == EAILODLevel::Maximum && MaximumLODCount > LODSettings.MaxMaximumLODAI) {
            // Keep AI in combat at Maximum LOD
            if (!AIData.bInCombat && !AIData.bIsEngagingPlayer) {
                SetAILODInternal(AIData, EAILODLevel::High);
                MaximumLODCount--;
                HighLODCount++;
            }
        }

        // Downgrade excess High LOD AI
        if (AIData.CurrentLODLevel == EAILODLevel::High && HighLODCount > LODSettings.MaxHighLODAI) {
            if (!AIData.bInCombat && !AIData.bIsEngagingPlayer) {
                SetAILODInternal(AIData, EAILODLevel::Standard);
                HighLODCount--;
            }
        }
    }
}

void UAILODManager::SetAILODInternal(FAILODData& AIData, EAILODLevel NewLODLevel)
{
    if (AIData.CurrentLODLevel == NewLODLevel || !AIData.AIController) {
        return;
    }

    const EAILODLevel OldLODLevel = AIData.CurrentLODLevel;
    AIData.CurrentLODLevel = NewLODLevel;

    // Apply LOD settings to AI
    if (UBehaviorTreeComponent* BTComp = AIData.AIController->GetBehaviorTreeComponent()) {
        switch (NewLODLevel) {
        case EAILODLevel::Inactive:
            BTComp->PauseLogic(TEXT("LOD_Inactive"));
            BTComp->SetComponentTickEnabled(false);
            break;

        case EAILODLevel::Minimal:
            BTComp->ResumeLogic(TEXT("LOD_Inactive"));
            BTComp->SetComponentTickEnabled(true);
            BTComp->SetComponentTickInterval(0.5f);
            break;

        case EAILODLevel::Standard:
            BTComp->ResumeLogic(TEXT("LOD_Inactive"));
            BTComp->SetComponentTickEnabled(true);
            BTComp->SetComponentTickInterval(0.1f);
            break;

        case EAILODLevel::High:
            BTComp->ResumeLogic(TEXT("LOD_Inactive"));
            BTComp->SetComponentTickEnabled(true);
            BTComp->SetComponentTickInterval(0.05f);
            break;

        case EAILODLevel::Maximum:
            BTComp->ResumeLogic(TEXT("LOD_Inactive"));
            BTComp->SetComponentTickEnabled(true);
            BTComp->SetComponentTickInterval(0.0f); // Every frame
            break;
        }
    }

    // Adjust AI Perception
    if (UAIPerceptionComponent* PerceptionComp = AIData.AIController->GetAIPerceptionComponent()) {
        switch (NewLODLevel) {
        case EAILODLevel::Inactive:
            PerceptionComp->SetComponentTickEnabled(false);
            break;

        case EAILODLevel::Minimal:
            PerceptionComp->SetComponentTickEnabled(true);
            PerceptionComp->SetComponentTickInterval(1.0f);
            break;

        case EAILODLevel::Standard:
            PerceptionComp->SetComponentTickEnabled(true);
            PerceptionComp->SetComponentTickInterval(0.2f);
            break;

        case EAILODLevel::High:
            PerceptionComp->SetComponentTickEnabled(true);
            PerceptionComp->SetComponentTickInterval(0.1f);
            break;

        case EAILODLevel::Maximum:
            PerceptionComp->SetComponentTickEnabled(true);
            PerceptionComp->SetComponentTickInterval(0.0f);
            break;
        }
    }

    // Adjust Animation LOD
    if (APawn* AIPawn = AIData.AIController->GetPawn()) {
        if (USkeletalMeshComponent* MeshComp = AIPawn->FindComponentByClass<USkeletalMeshComponent>()) {
            switch (NewLODLevel) {
            case EAILODLevel::Inactive:
                MeshComp->bPauseAnims = true;
                MeshComp->SetComponentTickEnabled(false);
                break;

            case EAILODLevel::Minimal:
                MeshComp->bPauseAnims = false;
                MeshComp->SetComponentTickEnabled(true);
                MeshComp->SetComponentTickInterval(0.1f);
                break;

            default:
                MeshComp->bPauseAnims = false;
                MeshComp->SetComponentTickEnabled(true);
                MeshComp->SetComponentTickInterval(0.0f);
                break;
            }
        }
    }

    // Broadcast event
    OnAILODChanged.Broadcast(AIData.AIController, NewLODLevel);

    UE_LOG(LogTemp, VeryVerbose, TEXT("AI LOD: %s changed from %d to %d (Distance: %.1f)"),
        *AIData.AIController->GetName(),
        static_cast<int32>(OldLODLevel),
        static_cast<int32>(NewLODLevel),
        AIData.DistanceToPlayer);
}

void UAILODManager::PredictPlayerMovement()
{
    if (!CachedPlayerPawn) {
        return;
    }

    const FVector CurrentPlayerPosition = CachedPlayerPawn->GetActorLocation();

    if (LastPlayerPosition != FVector::ZeroVector) {
        const FVector PlayerVelocity = (CurrentPlayerPosition - LastPlayerPosition) / LODSettings.LODUpdateFrequency;
        PredictedPlayerPosition = CurrentPlayerPosition + (PlayerVelocity * 2.0f); // Predict 2 seconds ahead
    } else {
        PredictedPlayerPosition = CurrentPlayerPosition;
    }

    LastPlayerPosition = CurrentPlayerPosition;
}

void UAILODManager::MonitorPerformance()
{
    if (UEngine* Engine = GEngine) {
        AverageFrameTime = Engine->GetMaxTickRate() > 0 ? 1000.0f / Engine->GetMaxTickRate() : 16.67f;

        // Auto-adjust LOD settings based on performance
        if (AverageFrameTime > 20.0f) // Below 50 FPS
        {
            LODSettings.MaxHighLODAI = FMath::Max(5, LODSettings.MaxHighLODAI - 1);
            LODSettings.MaxMaximumLODAI = FMath::Max(3, LODSettings.MaxMaximumLODAI - 1);
        } else if (AverageFrameTime < 14.0f) // Above 70 FPS
        {
            LODSettings.MaxHighLODAI = FMath::Min(20, LODSettings.MaxHighLODAI + 1);
            LODSettings.MaxMaximumLODAI = FMath::Min(12, LODSettings.MaxMaximumLODAI + 1);
        }
    }
}

void UAILODManager::CleanupInvalidAI()
{
    RegisteredAI.RemoveAll([](const FAILODData& Data) {
        return !Data.AIController || !IsValid(Data.AIController);
    });
}

EAILODLevel UAILODManager::CalculateOptimalLOD(const FAILODData& AIData) const
{
    // Always maximum LOD for AI in combat
    if (AIData.bInCombat || AIData.bIsEngagingPlayer) {
        return EAILODLevel::Maximum;
    }

    const float Distance = AIData.DistanceToPlayer;

    if (Distance <= LODSettings.MaximumDistance) {
        return EAILODLevel::Maximum;
    } else if (Distance <= LODSettings.HighDistance) {
        return EAILODLevel::High;
    } else if (Distance <= LODSettings.StandardDistance) {
        return EAILODLevel::Standard;
    } else if (Distance <= LODSettings.MinimalDistance) {
        return EAILODLevel::Minimal;
    } else {
        return EAILODLevel::Inactive;
    }
}

float UAILODManager::CalculateAIPriority(const FAILODData& AIData) const
{
    float Priority = 1.0f;

    // Combat AI gets highest priority
    if (AIData.bInCombat) {
        Priority += 10.0f;
    }

    // AI engaging player gets very high priority
    if (AIData.bIsEngagingPlayer) {
        Priority += 8.0f;
    }

    // Distance-based priority (closer = higher)
    const float MaxDistance = LODSettings.InactiveDistance;
    const float DistanceFactor = FMath::Clamp(1.0f - (AIData.DistanceToPlayer / MaxDistance), 0.0f, 1.0f);
    Priority += DistanceFactor * 5.0f;

    return Priority;
}

void UAILODManager::StartLODUpdateTimer()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(LODUpdateTimer, this,
            &UAILODManager::UpdateAILOD, LODSettings.LODUpdateFrequency, true);
    }
}

void UAILODManager::StopLODUpdateTimer()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(LODUpdateTimer);
    }
}