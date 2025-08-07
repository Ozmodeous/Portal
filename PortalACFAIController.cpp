// PortalACFAIController.cpp
#include "PortalACFAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionComponent.h"
#include "PortalAIDifficultyComponent.h"
#include "PortalAIFOVComponent.h"
#include "PortalAIHordeComponent.h"
#include "PortalAILearningComponent.h"

APortalACFAIController::APortalACFAIController()
{
    // Set tick enabled for component updates
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.05f; // 20 FPS tick rate for optimization

    // Create components
    DifficultyComponent = CreateDefaultSubobject<UPortalAIDifficultyComponent>(TEXT("DifficultyComponent"));
    LearningComponent = CreateDefaultSubobject<UPortalAILearningComponent>(TEXT("LearningComponent"));
    FOVComponent = CreateDefaultSubobject<UPortalAIFOVComponent>(TEXT("FOVComponent"));
    HordeComponent = CreateDefaultSubobject<UPortalAIHordeComponent>(TEXT("HordeComponent"));
}

void APortalACFAIController::BeginPlay()
{
    Super::BeginPlay();

    InitializeComponents();

    // Set initial difficulty from difficulty component
    if (DifficultyComponent) {
        CurrentDifficulty = DifficultyComponent->GetCurrentDifficulty();
    }

    UE_LOG(LogTemp, Log, TEXT("PortalACFAIController initialized for %s"),
        GetPawn() ? *GetPawn()->GetName() : TEXT("Unknown"));
}

void APortalACFAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!InPawn) {
        return;
    }

    // Initialize blackboard if we have one from ACF
    if (GetBlackboardComponent()) {
        // Set default blackboard values
        GetBlackboardComponent()->SetValueAsFloat("Difficulty", CurrentDifficulty);
        GetBlackboardComponent()->SetValueAsBool("IsInCombat", false);
        GetBlackboardComponent()->SetValueAsFloat("ReactionTime", BaseReactionTime);
    }

    // Initialize FOV component with possessed pawn
    if (FOVComponent) {
        FOVComponent->InitializeFOV(InPawn);
    }

    // Register with horde system if enabled
    if (bEnableHordeBehavior && HordeComponent) {
        HordeComponent->RegisterWithHorde();
    }

    OnAIStateChanged();
}

void APortalACFAIController::OnUnPossess()
{
    // Cleanup horde registration
    if (HordeComponent) {
        HordeComponent->UnregisterFromHorde();
    }

    // Save learning data before unpossess
    if (LearningComponent && bEnableLearning) {
        LearningComponent->SaveLearningData();
    }

    Super::OnUnPossess();
}

void APortalACFAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Throttled component updates for performance
    if (GetWorld()->GetTimeSeconds() - LastComponentUpdateTime >= ComponentUpdateInterval) {
        UpdateComponents(DeltaTime);
        LastComponentUpdateTime = GetWorld()->GetTimeSeconds();
    }

    // FOV component needs more frequent updates for smooth visibility
    if (FOVComponent && FOVComponent->IsEnabled()) {
        FOVComponent->UpdateFOV(DeltaTime);
    }
}

void APortalACFAIController::UpdateComponents(float DeltaTime)
{
    // Update difficulty-based parameters
    if (DifficultyComponent) {
        DifficultyComponent->UpdateDifficulty(DeltaTime);

        // Apply difficulty modifiers to ACF systems
        float NewDifficulty = DifficultyComponent->GetCurrentDifficulty();
        if (FMath::Abs(NewDifficulty - CurrentDifficulty) > 0.01f) {
            SetDifficulty(NewDifficulty);
        }
    }

    // Process learning if enabled and in combat
    if (LearningComponent && bEnableLearning && bIsInCombat) {
        AActor* Target = GetBlackboardComponent() ? Cast<AActor>(GetBlackboardComponent()->GetValueAsObject("Target")) : nullptr;

        if (Target) {
            LearningComponent->ProcessCombatData(Target, DeltaTime);
        }
    }

    // Update horde behavior
    if (HordeComponent && bEnableHordeBehavior) {
        HordeComponent->UpdateHordeBehavior(DeltaTime);
    }
}

void APortalACFAIController::InitializeComponents()
{
    // Initialize difficulty component
    if (DifficultyComponent) {
        DifficultyComponent->Initialize(this);
    }

    // Load learning data
    if (LearningComponent && bEnableLearning) {
        LearningComponent->Initialize(this);
        LearningComponent->LoadLearningData();
    }

    // Setup FOV system
    if (FOVComponent) {
        FOVComponent->Initialize(this);
    }

    // Initialize horde component
    if (HordeComponent) {
        HordeComponent->Initialize(this);
    }
}

void APortalACFAIController::UpdateAIState()
{
    // Check combat state from ACF systems
    bool bWasInCombat = bIsInCombat;

    // Use ACF's built-in combat detection
    AActor* CurrentTarget = nullptr;
    if (GetBlackboardComponent()) {
        CurrentTarget = Cast<AActor>(GetBlackboardComponent()->GetValueAsObject("Target"));
    }

    bIsInCombat = (CurrentTarget != nullptr);

    // Handle state transitions
    if (bIsInCombat && !bWasInCombat) {
        OnTargetAcquired(CurrentTarget);

        // Notify components
        if (LearningComponent) {
            LearningComponent->OnCombatStarted(CurrentTarget);
        }
        if (HordeComponent) {
            HordeComponent->OnEngageTarget(CurrentTarget);
        }
    } else if (!bIsInCombat && bWasInCombat) {
        OnTargetLost();

        // Notify components
        if (LearningComponent) {
            LearningComponent->OnCombatEnded();
        }
        if (HordeComponent) {
            HordeComponent->OnDisengageTarget();
        }
    }

    // Update blackboard
    if (GetBlackboardComponent()) {
        GetBlackboardComponent()->SetValueAsBool("IsInCombat", bIsInCombat);
    }

    OnAIStateChanged();
}

void APortalACFAIController::SetDifficulty(float NewDifficulty)
{
    CurrentDifficulty = FMath::Clamp(NewDifficulty, 0.1f, 10.0f);

    // Update all components with new difficulty
    if (DifficultyComponent) {
        DifficultyComponent->SetDifficulty(CurrentDifficulty);
    }

    // Adjust reaction time based on difficulty
    float AdjustedReactionTime = BaseReactionTime / CurrentDifficulty;

    // Update blackboard
    if (GetBlackboardComponent()) {
        GetBlackboardComponent()->SetValueAsFloat("Difficulty", CurrentDifficulty);
        GetBlackboardComponent()->SetValueAsFloat("ReactionTime", AdjustedReactionTime);
    }

    // Notify ACF systems of difficulty change
    // This would integrate with ACF's stat modifiers
    if (APawn* ControlledPawn = GetPawn()) {
        // Apply difficulty-based stat modifiers through ACF systems
        // This is where you'd integrate with ACF's character stats
    }

    UE_LOG(LogTemp, Verbose, TEXT("AI Difficulty set to %.2f"), CurrentDifficulty);
}