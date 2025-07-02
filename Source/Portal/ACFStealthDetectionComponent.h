#pragma once

#include "ACFAIController.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Perception/AIPerceptionTypes.h"
#include "ACFStealthDetectionComponent.generated.h"

class UPortalStealthConfigDataAsset;
class ULightComponent;
class UPointLightComponent;
class USpotLightComponent;

USTRUCT(BlueprintType)
struct FHybridStealthSettings {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Detection")
    bool bInstantAggroInLight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Detection")
    float LightAggroRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Detection")
    float LightDetectionRadius = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Darkness Detection")
    float DarknessVisualRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Darkness Detection")
    float DarknessAudioRange = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Detection")
    float WalkingNoiseRange = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Detection")
    float RunningNoiseRange = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Detection")
    float CrouchingNoiseRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Detection")
    float GrassNoiseMultiplier = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Detection")
    float VegetationNoiseMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro Behavior")
    float AggroAlertRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro Behavior")
    float SoundInvestigationDuration = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro Behavior")
    bool bAlertOtherGuardsOnLightDetection = true;

    FHybridStealthSettings()
    {
        bInstantAggroInLight = true;
        LightAggroRange = 1200.0f;
        LightDetectionRadius = 600.0f;
        DarknessVisualRange = 150.0f;
        DarknessAudioRange = 600.0f;
        WalkingNoiseRange = 300.0f;
        RunningNoiseRange = 800.0f;
        CrouchingNoiseRange = 150.0f;
        GrassNoiseMultiplier = 1.2f;
        VegetationNoiseMultiplier = 1.5f;
        AggroAlertRadius = 1000.0f;
        SoundInvestigationDuration = 8.0f;
        bAlertOtherGuardsOnLightDetection = true;
    }
};

UENUM(BlueprintType)
enum class EStealthDetectionType : uint8 {
    None,
    Visual,
    Audio,
    LightAggro
};

UCLASS(ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class PORTAL_API UACFStealthDetectionComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UACFStealthDetectionComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Main Detection Function - Called by ACF AI Controller
    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    bool PerformStealthDetection(APawn* TargetPlayer, float& OutDetectionRange, EStealthDetectionType& OutDetectionType);

    // Individual Detection Methods
    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    bool IsPlayerIlluminated(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    bool CanHearPlayer(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    float CalculatePlayerNoiseLevel(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    bool IsPlayerInVegetation(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    bool ShouldUseDarknessDetection(APawn* Player);

    // Action Methods
    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    void TriggerLightAggro(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    void AlertNearbyGuards(APawn* Player);

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    void StartSoundInvestigation(FVector SoundLocation);

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    void ApplyStealthConfiguration();

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    void SetStealthConfigAsset(UPortalStealthConfigDataAsset* NewConfigAsset);

    UFUNCTION(BlueprintCallable, Category = "Stealth Detection")
    void OverrideACFDetection(bool bEnable);

    // Getters
    UFUNCTION(BlueprintPure, Category = "Stealth Detection")
    EStealthDetectionType GetLastDetectionType() const { return LastDetectionType; }

    UFUNCTION(BlueprintPure, Category = "Stealth Detection")
    bool IsInvestigatingSound() const { return bInvestigatingSound; }

    UFUNCTION(BlueprintPure, Category = "Stealth Detection")
    FHybridStealthSettings GetStealthSettings() const { return StealthSettings; }

protected:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth Configuration")
    TObjectPtr<UPortalStealthConfigDataAsset> StealthConfigAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth Detection")
    FHybridStealthSettings StealthSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth Detection")
    bool bEnableStealthDetection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth Detection")
    bool bOverrideACFDetection = true;

    // State
    UPROPERTY(BlueprintReadOnly, Category = "Stealth Detection")
    EStealthDetectionType LastDetectionType;

    UPROPERTY(BlueprintReadOnly, Category = "Stealth Detection")
    bool bInvestigatingSound;

    UPROPERTY(BlueprintReadOnly, Category = "Stealth Detection")
    TArray<APawn*> PlayersInHearingRange;

    // ACF Integration
    UPROPERTY(BlueprintReadOnly, Category = "ACF Integration")
    TObjectPtr<AACFAIController> ACFController;

    UPROPERTY(BlueprintReadOnly, Category = "ACF Integration")
    TObjectPtr<APawn> OwnerPawn;

    // Environment
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TArray<FName> VegetationTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TArray<FName> GrassTags;

private:
    FTimerHandle SoundInvestigationTimer;

    // Bound to ACF AI Controller's perception system
    UFUNCTION()
    void OnACFPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    // Internal detection methods
    void CheckForStealthThreats();
    bool HasLineOfSightToPlayer(APawn* Player);
    void NotifyACFControllerOfDetection(APawn* Player, EStealthDetectionType DetectionType);
    void InitializeWithACFController();

    UFUNCTION()
    void OnSoundInvestigationComplete();
};