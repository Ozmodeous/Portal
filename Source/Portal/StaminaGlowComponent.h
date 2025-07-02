#pragma once

#include "Components/ActorComponent.h"
#include "Components/LightComponent.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameplayTags.h"
#include "StaminaGlowComponent.generated.h"

class UARSStatisticsComponent;

UCLASS(ClassGroup = (Portal), meta = (BlueprintSpawnableComponent))
class PORTAL_API UStaminaGlowComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UStaminaGlowComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    UFUNCTION(BlueprintCallable, Category = "Stamina Glow")
    void SetStamina(float CurrentStamina, float MaxStamina);

    UFUNCTION(BlueprintCallable, Category = "Stamina Glow")
    void SetStaminaPercent(float StaminaPercent);

    UFUNCTION(BlueprintCallable, Category = "Stamina Glow")
    void EnableGlow(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Stamina Glow")
    void SetGlowThreshold(float Threshold);

    UFUNCTION(BlueprintCallable, Category = "Stamina Glow")
    void SetBrightnessRange(float MinBrightness, float MaxBrightness);

    UFUNCTION(BlueprintCallable, Category = "Stamina Glow")
    void SetGlowColor(FLinearColor Color);

    UFUNCTION(BlueprintCallable, Category = "Stamina Glow")
    void SetStaminaTag(FGameplayTag NewStaminaTag);

    UFUNCTION(BlueprintPure, Category = "Stamina Glow")
    bool IsGlowing() const { return bIsGlowing; }

    UFUNCTION(BlueprintPure, Category = "Stamina Glow")
    float GetCurrentStaminaPercent() const { return CurrentStaminaPercent; }

    UFUNCTION(BlueprintPure, Category = "Stamina Glow")
    bool HasGlowLight() const { return GlowLight != nullptr; }

    UFUNCTION(BlueprintPure, Category = "Stamina Glow")
    bool HasStatsComponent() const { return StatsComponent != nullptr; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glow Settings")
    float GlowThreshold;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glow Settings")
    float MinLightIntensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glow Settings")
    float MaxLightIntensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glow Settings")
    FLinearColor GlowColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glow Settings")
    float UpdateInterval;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glow Settings")
    bool bAutoReadStamina;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glow Settings", meta = (Categories = "RPG.Resources"))
    FGameplayTag StaminaTag;

    UPROPERTY(BlueprintReadOnly, Category = "Glow State")
    float CurrentStaminaPercent;

    UPROPERTY(BlueprintReadOnly, Category = "Glow State")
    bool bIsGlowing;

    UPROPERTY(BlueprintReadOnly, Category = "Glow State")
    bool bGlowEnabled;

    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TObjectPtr<ULightComponent> GlowLight;

    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TObjectPtr<UARSStatisticsComponent> StatsComponent;

    float OriginalIntensity;

private:
    FTimerHandle UpdateTimer;

    void UpdateGlow();
    void FindGlowLight();
    void FindStatsComponent();
    void ReadStaminaFromARS();

    UFUNCTION()
    void OnUpdateTimer();
};