#include "StaminaGlowComponent.h"
#include "ARSStatisticsComponent.h"
#include "Components/LightComponent.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"

UStaminaGlowComponent::UStaminaGlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    GlowThreshold = 0.8f;
    MinLightIntensity = 0.0f;
    MaxLightIntensity = 2000.0f;
    GlowColor = FLinearColor::Red;
    UpdateInterval = 0.2f;
    bAutoReadStamina = true;
    StaminaTag = FGameplayTag::RequestGameplayTag(FName("RPG.Resources.Stamina"));

    CurrentStaminaPercent = 1.0f;
    bIsGlowing = false;
    bGlowEnabled = true;
    OriginalIntensity = 0.0f;
    GlowLight = nullptr;
    StatsComponent = nullptr;
}

void UStaminaGlowComponent::BeginPlay()
{
    Super::BeginPlay();

    FindGlowLight();
    FindStatsComponent();

    if (bAutoReadStamina && UpdateInterval > 0.0f && StatsComponent) {
        GetWorld()->GetTimerManager().SetTimer(
            UpdateTimer,
            this,
            &UStaminaGlowComponent::OnUpdateTimer,
            UpdateInterval,
            true);
    }
}

void UStaminaGlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(UpdateTimer);
    }
    Super::EndPlay(EndPlayReason);
}

void UStaminaGlowComponent::SetStamina(float CurrentStamina, float MaxStamina)
{
    if (MaxStamina > 0.0f) {
        SetStaminaPercent(CurrentStamina / MaxStamina);
    }
}

void UStaminaGlowComponent::SetStaminaPercent(float StaminaPercent)
{
    CurrentStaminaPercent = FMath::Clamp(StaminaPercent, 0.0f, 1.0f);
    UpdateGlow();
}

void UStaminaGlowComponent::EnableGlow(bool bEnable)
{
    bGlowEnabled = bEnable;
    UpdateGlow();
}

void UStaminaGlowComponent::SetGlowThreshold(float Threshold)
{
    GlowThreshold = FMath::Clamp(Threshold, 0.0f, 1.0f);
    UpdateGlow();
}

void UStaminaGlowComponent::SetBrightnessRange(float MinBrightness, float MaxBrightness)
{
    MinLightIntensity = FMath::Max(0.0f, MinBrightness);
    MaxLightIntensity = FMath::Max(MinLightIntensity, MaxBrightness);
    UpdateGlow();
}

void UStaminaGlowComponent::SetGlowColor(FLinearColor Color)
{
    GlowColor = Color;
    if (GlowLight) {
        GlowLight->SetLightColor(GlowColor);
    }
}

void UStaminaGlowComponent::SetStaminaTag(FGameplayTag NewStaminaTag)
{
    StaminaTag = NewStaminaTag;
}

void UStaminaGlowComponent::UpdateGlow()
{
    if (!GlowLight || !bGlowEnabled) {
        return;
    }

    bool bShouldGlow = CurrentStaminaPercent <= GlowThreshold;

    if (bShouldGlow) {
        bIsGlowing = true;

        float GlowStrength = 1.0f - (CurrentStaminaPercent / GlowThreshold);
        GlowStrength = FMath::Clamp(GlowStrength, 0.0f, 1.0f);

        float LightIntensity = FMath::Lerp(MinLightIntensity, MaxLightIntensity, GlowStrength);

        GlowLight->SetIntensity(LightIntensity);
        GlowLight->SetVisibility(true);
    } else {
        bIsGlowing = false;
        GlowLight->SetIntensity(OriginalIntensity);
        GlowLight->SetVisibility(OriginalIntensity > 0.0f);
    }
}

void UStaminaGlowComponent::FindGlowLight()
{
    if (AActor* Owner = GetOwner()) {
        TArray<UActorComponent*> Components = Owner->GetComponents().Array();

        for (UActorComponent* Component : Components) {
            if (ULightComponent* Light = Cast<ULightComponent>(Component)) {
                if (Light->GetName().Contains(TEXT("GlowLight"))) {
                    GlowLight = Light;
                    OriginalIntensity = Light->Intensity;
                    Light->SetLightColor(GlowColor);
                    Light->SetIntensity(OriginalIntensity);
                    Light->SetVisibility(OriginalIntensity > 0.0f);

                    UE_LOG(LogTemp, Log, TEXT("Found GlowLight: %s"), *Light->GetName());
                    return;
                }
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("No GlowLight found on %s"), *Owner->GetName());
    }
}

void UStaminaGlowComponent::FindStatsComponent()
{
    if (AActor* Owner = GetOwner()) {
        StatsComponent = Owner->FindComponentByClass<UARSStatisticsComponent>();

        if (StatsComponent) {
            UE_LOG(LogTemp, Log, TEXT("Found ARSStatisticsComponent on %s"), *Owner->GetName());
        } else {
            UE_LOG(LogTemp, Warning, TEXT("No ARSStatisticsComponent found on %s"), *Owner->GetName());
        }
    }
}

void UStaminaGlowComponent::ReadStaminaFromARS()
{
    if (!StatsComponent || !StaminaTag.IsValid()) {
        return;
    }

    // Check if the stamina statistic exists
    if (!StatsComponent->HasValidStatistic(StaminaTag)) {
        UE_LOG(LogTemp, Warning, TEXT("Stamina tag %s not found in statistics"), *StaminaTag.ToString());
        return;
    }

    // Get current and max stamina using ARS API
    float CurrentStamina = StatsComponent->GetCurrentValueForStatitstic(StaminaTag);
    float MaxStamina = StatsComponent->GetMaxValueForStatitstic(StaminaTag);

    // Calculate percentage
    if (MaxStamina > 0.0f) {
        float NewStaminaPercent = CurrentStamina / MaxStamina;
        if (FMath::Abs(NewStaminaPercent - CurrentStaminaPercent) > 0.01f) {
            SetStaminaPercent(NewStaminaPercent);
        }
    }
}

void UStaminaGlowComponent::OnUpdateTimer()
{
    if (bAutoReadStamina) {
        ReadStaminaFromARS();
    }
}