// PortalAIDifficultyComponent.cpp
#include "PortalAIDifficultyComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "PortalACFAIController.h"

UPortalAIDifficultyComponent::UPortalAIDifficultyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Set default difficulty settings
    CurrentSettings = FPortalAIDifficultySettings();
    CurrentDifficulty = BaseDifficulty;
    TargetDifficulty = BaseDifficulty;
}

void UPortalAIDifficultyComponent::BeginPlay()
{
    Super::BeginPlay();

    // Load initial difficulty settings
    CalculateDifficultySettings();
}

void UPortalAIDifficultyComponent::Initialize(APortalACFAIController* Controller)
{
    OwnerController = Controller;

    if (!OwnerController) {
        UE_LOG(LogTemp, Warning, TEXT("PortalAIDifficultyComponent: No valid controller provided"));
        return;
    }

    // Apply initial difficulty settings
    ApplySettingsToController();

    UE_LOG(LogTemp, Log, TEXT("Difficulty Component initialized with level %.2f"), CurrentDifficulty);
}

void UPortalAIDifficultyComponent::SetDifficulty(float NewDifficulty)
{
    float ClampedDifficulty = FMath::Clamp(NewDifficulty, MinDifficulty, MaxDifficulty);

    if (bUseDynamicDifficulty) {
        // Smooth transition to new difficulty
        TargetDifficulty = ClampedDifficulty;
    } else {
        // Immediate difficulty change
        CurrentDifficulty = ClampedDifficulty;
        TargetDifficulty = ClampedDifficulty;
        CalculateDifficultySettings();
        ApplySettingsToController();
    }

    OnDifficultyChanged(CurrentDifficulty);
}

void UPortalAIDifficultyComponent::SetDifficultyPreset(FName PresetName)
{
    if (!DifficultyPresetsTable) {
        UE_LOG(LogTemp, Warning, TEXT("No difficulty presets table assigned"));
        return;
    }

    // Find preset in data table
    FPortalAIDifficultySettings* PresetSettings = DifficultyPresetsTable->FindRow<FPortalAIDifficultySettings>(PresetName, TEXT("Difficulty Preset Lookup"));

    if (PresetSettings) {
        CurrentSettings = *PresetSettings;
        CurrentDifficulty = PresetSettings->DifficultyLevel;
        TargetDifficulty = CurrentDifficulty;

        ApplySettingsToController();
        OnDifficultyPresetApplied(CurrentSettings);

        UE_LOG(LogTemp, Log, TEXT("Applied difficulty preset: %s"), *PresetName.ToString());
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Difficulty preset not found: %s"), *PresetName.ToString());
    }
}

void UPortalAIDifficultyComponent::UpdateDifficulty(float DeltaTime)
{
    if (!bUseDynamicDifficulty) {
        return;
    }

    // Smoothly interpolate to target difficulty
    if (FMath::Abs(CurrentDifficulty - TargetDifficulty) > 0.01f) {
        InterpolateDifficulty(DeltaTime);
        CalculateDifficultySettings();
        ApplySettingsToController();
    }
}

void UPortalAIDifficultyComponent::AdjustDifficultyBasedOnPerformance(float PlayerPerformanceScore)
{
    if (!bUseDynamicDifficulty) {
        return;
    }

    // Adjust difficulty based on player performance
    // High performance = increase difficulty, Low performance = decrease difficulty
    float DifficultyAdjustment = 0.0f;

    if (PlayerPerformanceScore > 0.8f) // Player doing very well
    {
        DifficultyAdjustment = DifficultyAdjustmentRate * 2.0f;
    } else if (PlayerPerformanceScore > 0.6f) // Player doing well
    {
        DifficultyAdjustment = DifficultyAdjustmentRate;
    } else if (PlayerPerformanceScore < 0.3f) // Player struggling
    {
        DifficultyAdjustment = -DifficultyAdjustmentRate * 2.0f;
    } else if (PlayerPerformanceScore < 0.5f) // Player having some difficulty
    {
        DifficultyAdjustment = -DifficultyAdjustmentRate;
    }

    if (DifficultyAdjustment != 0.0f) {
        TargetDifficulty = FMath::Clamp(TargetDifficulty + DifficultyAdjustment, MinDifficulty, MaxDifficulty);
        UE_LOG(LogTemp, Verbose, TEXT("Adjusted target difficulty to %.2f based on performance"), TargetDifficulty);
    }
}

void UPortalAIDifficultyComponent::IncreaseDifficulty(float Amount)
{
    SetDifficulty(CurrentDifficulty + Amount);
}

void UPortalAIDifficultyComponent::DecreaseDifficulty(float Amount)
{
    SetDifficulty(CurrentDifficulty - Amount);
}

void UPortalAIDifficultyComponent::ApplyDifficultyToACFCharacter(AACFCharacter* Character)
{
    if (!Character) {
        return;
    }

    // Apply stat multipliers to ACF character
    // This would integrate with ACF's stat system

    // Example: Modify movement speed
    if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement()) {
        MovementComp->MaxWalkSpeed = MovementComp->GetMaxSpeed() * CurrentSettings.SpeedMultiplier;
    }

    // Here you would integrate with ACF's specific stat modification system
    // For example:
    // - Modify ACF health component
    // - Adjust ACF damage multipliers
    // - Update ACF combat stats

    UE_LOG(LogTemp, Verbose, TEXT("Applied difficulty settings to ACF Character: %s"), *Character->GetName());
}

void UPortalAIDifficultyComponent::ApplyDifficultyToPerception()
{
    if (!OwnerController) {
        return;
    }

    // Get AI perception component
    UAIPerceptionComponent* PerceptionComp = OwnerController->GetPerceptionComponent();
    if (!PerceptionComp) {
        return;
    }

    // Apply perception modifiers based on difficulty
    // This would modify sight radius, peripheral vision, etc.

    // Note: In production, you'd modify the actual perception configuration
    // For ACF integration, this might involve modifying ACF-specific perception settings

    UE_LOG(LogTemp, Verbose, TEXT("Applied difficulty to perception: Detection Range x%.2f"),
        CurrentSettings.DetectionRangeMultiplier);
}

void UPortalAIDifficultyComponent::ApplyDifficultyToBehavior()
{
    if (!OwnerController || !OwnerController->GetBlackboardComponent()) {
        return;
    }

    UBlackboardComponent* BlackboardComp = OwnerController->GetBlackboardComponent();

    // Update blackboard values for behavior tree
    BlackboardComp->SetValueAsFloat("ReactionTime", CurrentSettings.ReactionTimeMultiplier);
    BlackboardComp->SetValueAsFloat("Accuracy", CurrentSettings.AccuracyPercentage / 100.0f);
    BlackboardComp->SetValueAsFloat("Aggression", CurrentSettings.AggressionLevel);

    // Set tactical capabilities
    BlackboardComp->SetValueAsBool("CanFlank", CurrentSettings.bCanFlank);
    BlackboardComp->SetValueAsBool("CanRetreat", CurrentSettings.bCanRetreat);
    BlackboardComp->SetValueAsBool("CanCallReinforcements", CurrentSettings.bCanCallReinforcements);
    BlackboardComp->SetValueAsBool("UsesAdvancedCover", CurrentSettings.bUsesAdvancedCover);

    UE_LOG(LogTemp, Verbose, TEXT("Applied difficulty to behavior: Aggression %.2f, Accuracy %.1f%%"),
        CurrentSettings.AggressionLevel, CurrentSettings.AccuracyPercentage);
}

void UPortalAIDifficultyComponent::InterpolateDifficulty(float DeltaTime)
{
    // Smooth difficulty transition
    float InterpSpeed = 1.0f; // 1 second to reach target
    CurrentDifficulty = FMath::FInterpTo(CurrentDifficulty, TargetDifficulty, DeltaTime, InterpSpeed);
}

void UPortalAIDifficultyComponent::CalculateDifficultySettings()
{
    // If we have preset data, try to interpolate between presets
    if (DifficultyPresetsTable) {
        LoadPresetSettings(CurrentDifficulty);
    } else {
        // Calculate settings based on difficulty level
        float DifficultyRatio = (CurrentDifficulty - MinDifficulty) / (MaxDifficulty - MinDifficulty);

        CurrentSettings.DifficultyLevel = CurrentDifficulty;
        CurrentSettings.HealthMultiplier = FMath::Lerp(0.5f, 2.0f, DifficultyRatio);
        CurrentSettings.DamageMultiplier = FMath::Lerp(0.5f, 2.0f, DifficultyRatio);
        CurrentSettings.SpeedMultiplier = FMath::Lerp(0.8f, 1.5f, DifficultyRatio);
        CurrentSettings.DetectionRangeMultiplier = FMath::Lerp(0.7f, 1.5f, DifficultyRatio);
        CurrentSettings.PeripheralVisionAngle = FMath::Lerp(60.0f, 120.0f, DifficultyRatio);
        CurrentSettings.ReactionTimeMultiplier = FMath::Lerp(2.0f, 0.3f, DifficultyRatio);
        CurrentSettings.AccuracyPercentage = FMath::Lerp(30.0f, 95.0f, DifficultyRatio);
        CurrentSettings.AggressionLevel = FMath::Lerp(0.2f, 1.0f, DifficultyRatio);

        // Enable tactics at higher difficulties
        CurrentSettings.bCanFlank = (CurrentDifficulty >= 2.0f);
        CurrentSettings.bCanRetreat = (CurrentDifficulty >= 1.5f);
        CurrentSettings.bCanCallReinforcements = (CurrentDifficulty >= 3.0f);
        CurrentSettings.bUsesAdvancedCover = (CurrentDifficulty >= 2.5f);
    }
}

void UPortalAIDifficultyComponent::LoadPresetSettings(float DifficultyLevel)
{
    if (!DifficultyPresetsTable) {
        return;
    }

    // Find the two closest presets and interpolate between them
    TArray<FPortalAIDifficultySettings*> AllPresets;
    DifficultyPresetsTable->GetAllRows<FPortalAIDifficultySettings>(TEXT("Difficulty Presets"), AllPresets);

    if (AllPresets.Num() == 0) {
        return;
    }

    // Sort presets by difficulty level
    AllPresets.Sort([](const FPortalAIDifficultySettings& A, const FPortalAIDifficultySettings& B) {
        return A.DifficultyLevel < B.DifficultyLevel;
    });

    // Find bracketing presets
    FPortalAIDifficultySettings* LowerPreset = nullptr;
    FPortalAIDifficultySettings* UpperPreset = nullptr;

    for (int32 i = 0; i < AllPresets.Num(); i++) {
        if (AllPresets[i]->DifficultyLevel <= DifficultyLevel) {
            LowerPreset = AllPresets[i];
        }

        if (AllPresets[i]->DifficultyLevel >= DifficultyLevel && !UpperPreset) {
            UpperPreset = AllPresets[i];
            break;
        }
    }

    // Apply or interpolate settings
    if (LowerPreset && UpperPreset && LowerPreset != UpperPreset) {
        // Interpolate between presets
        float Alpha = (DifficultyLevel - LowerPreset->DifficultyLevel) / (UpperPreset->DifficultyLevel - LowerPreset->DifficultyLevel);

        CurrentSettings.DifficultyLevel = DifficultyLevel;
        CurrentSettings.HealthMultiplier = FMath::Lerp(LowerPreset->HealthMultiplier, UpperPreset->HealthMultiplier, Alpha);
        CurrentSettings.DamageMultiplier = FMath::Lerp(LowerPreset->DamageMultiplier, UpperPreset->DamageMultiplier, Alpha);
        CurrentSettings.SpeedMultiplier = FMath::Lerp(LowerPreset->SpeedMultiplier, UpperPreset->SpeedMultiplier, Alpha);
        CurrentSettings.DetectionRangeMultiplier = FMath::Lerp(LowerPreset->DetectionRangeMultiplier, UpperPreset->DetectionRangeMultiplier, Alpha);
        CurrentSettings.PeripheralVisionAngle = FMath::Lerp(LowerPreset->PeripheralVisionAngle, UpperPreset->PeripheralVisionAngle, Alpha);
        CurrentSettings.ReactionTimeMultiplier = FMath::Lerp(LowerPreset->ReactionTimeMultiplier, UpperPreset->ReactionTimeMultiplier, Alpha);
        CurrentSettings.AccuracyPercentage = FMath::Lerp(LowerPreset->AccuracyPercentage, UpperPreset->AccuracyPercentage, Alpha);
        CurrentSettings.AggressionLevel = FMath::Lerp(LowerPreset->AggressionLevel, UpperPreset->AggressionLevel, Alpha);

        // Boolean values use threshold
        CurrentSettings.bCanFlank = (Alpha > 0.5f) ? UpperPreset->bCanFlank : LowerPreset->bCanFlank;
        CurrentSettings.bCanRetreat = (Alpha > 0.5f) ? UpperPreset->bCanRetreat : LowerPreset->bCanRetreat;
        CurrentSettings.bCanCallReinforcements = (Alpha > 0.5f) ? UpperPreset->bCanCallReinforcements : LowerPreset->bCanCallReinforcements;
        CurrentSettings.bUsesAdvancedCover = (Alpha > 0.5f) ? UpperPreset->bUsesAdvancedCover : LowerPreset->bUsesAdvancedCover;
    } else if (LowerPreset) {
        CurrentSettings = *LowerPreset;
    } else if (UpperPreset) {
        CurrentSettings = *UpperPreset;
    }
}

void UPortalAIDifficultyComponent::ApplySettingsToController()
{
    if (!OwnerController) {
        return;
    }

    // Apply to ACF character if possessed
    if (APawn* ControlledPawn = OwnerController->GetPawn()) {
        if (AACFCharacter* ACFChar = Cast<AACFCharacter>(ControlledPawn)) {
            ApplyDifficultyToACFCharacter(ACFChar);
        }
    }

    // Apply to perception system
    ApplyDifficultyToPerception();

    // Apply to behavior system
    ApplyDifficultyToBehavior();
}