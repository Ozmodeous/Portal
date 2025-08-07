// PortalAIDifficultyComponent.h
#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PortalAIDifficultyComponent.generated.h"

// Forward declarations
class APortalACFAIController;
class AACFCharacter;

/**
 * Difficulty settings structure for AI behavior modification
 */
USTRUCT(BlueprintType)
struct FPortalAIDifficultySettings : public FTableRowBase {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float DifficultyLevel = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float HealthMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float SpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
    float DetectionRangeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
    float PeripheralVisionAngle = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float ReactionTimeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float AccuracyPercentage = 75.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float AggressionLevel = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    bool bCanFlank = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    bool bCanRetreat = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    bool bCanCallReinforcements = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactics")
    bool bUsesAdvancedCover = false;

    FPortalAIDifficultySettings()
    {
        DifficultyLevel = 1.0f;
        HealthMultiplier = 1.0f;
        DamageMultiplier = 1.0f;
        SpeedMultiplier = 1.0f;
        DetectionRangeMultiplier = 1.0f;
        PeripheralVisionAngle = 90.0f;
        ReactionTimeMultiplier = 1.0f;
        AccuracyPercentage = 75.0f;
        AggressionLevel = 0.5f;
        bCanFlank = false;
        bCanRetreat = false;
        bCanCallReinforcements = false;
        bUsesAdvancedCover = false;
    }
};

/**
 * Difficulty Component for Portal AI
 *
 * Manages dynamic difficulty adjustment and AI stat modifiers
 * Integrates with ACF's stat system for seamless difficulty scaling
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UPortalAIDifficultyComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UPortalAIDifficultyComponent();

protected:
    virtual void BeginPlay() override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty Config")
    float BaseDifficulty = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty Config")
    float MinDifficulty = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty Config")
    float MaxDifficulty = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty Config")
    bool bUseDynamicDifficulty = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty Config")
    float DifficultyAdjustmentRate = 0.1f;

    // Difficulty presets data table
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty Config")
    UDataTable* DifficultyPresetsTable;

    // Current settings
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Difficulty State")
    FPortalAIDifficultySettings CurrentSettings;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Difficulty State")
    float CurrentDifficulty = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Difficulty State")
    float TargetDifficulty = 1.0f;

public:
    // Initialization
    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void Initialize(APortalACFAIController* Controller);

    // Difficulty management
    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void SetDifficulty(float NewDifficulty);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void SetDifficultyPreset(FName PresetName);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void UpdateDifficulty(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    float GetCurrentDifficulty() const { return CurrentDifficulty; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    FPortalAIDifficultySettings GetCurrentSettings() const { return CurrentSettings; }

    // Dynamic difficulty adjustment
    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void AdjustDifficultyBasedOnPerformance(float PlayerPerformanceScore);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void IncreaseDifficulty(float Amount = 0.1f);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void DecreaseDifficulty(float Amount = 0.1f);

    // ACF Integration
    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void ApplyDifficultyToACFCharacter(AACFCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void ApplyDifficultyToPerception();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Difficulty")
    void ApplyDifficultyToBehavior();

    // Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Difficulty")
    void OnDifficultyChanged(float NewDifficulty);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Difficulty")
    void OnDifficultyPresetApplied(const FPortalAIDifficultySettings& Settings);

private:
    UPROPERTY()
    APortalACFAIController* OwnerController;

    // Internal methods
    void InterpolateDifficulty(float DeltaTime);
    void CalculateDifficultySettings();
    void LoadPresetSettings(float DifficultyLevel);
    void ApplySettingsToController();
};