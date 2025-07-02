#include "PortalStealthConfigDataAsset.h"
#include "Engine/Engine.h"

UPortalStealthConfigDataAsset::UPortalStealthConfigDataAsset()
{
    // Initialize default stealth settings
    StealthSettings = FHybridStealthSettings();

    // Default vegetation tags
    VegetationTags.Add(FName("Vegetation"));
    VegetationTags.Add(FName("Tree"));
    VegetationTags.Add(FName("Bush"));
    VegetationTags.Add(FName("Foliage"));

    // Default grass tags
    GrassTags.Add(FName("Grass"));
    GrassTags.Add(FName("LongGrass"));
    GrassTags.Add(FName("Weeds"));
}

void UPortalStealthConfigDataAsset::PostLoad()
{
    Super::PostLoad();

    // Validate settings on load
    if (StealthSettings.LightAggroRange <= 0.0f) {
        UE_LOG(LogTemp, Warning, TEXT("Portal Stealth Config: LightAggroRange must be greater than 0"));
        StealthSettings.LightAggroRange = 1200.0f;
    }

    if (StealthSettings.LightDetectionRadius <= 0.0f) {
        UE_LOG(LogTemp, Warning, TEXT("Portal Stealth Config: LightDetectionRadius must be greater than 0"));
        StealthSettings.LightDetectionRadius = 600.0f;
    }
}

#if WITH_EDITOR
void UPortalStealthConfigDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Validate changes made in editor
    if (PropertyChangedEvent.Property) {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(FHybridStealthSettings, LightAggroRange)) {
            if (StealthSettings.LightAggroRange <= 0.0f) {
                StealthSettings.LightAggroRange = 1200.0f;
            }
        } else if (PropertyName == GET_MEMBER_NAME_CHECKED(FHybridStealthSettings, LightDetectionRadius)) {
            if (StealthSettings.LightDetectionRadius <= 0.0f) {
                StealthSettings.LightDetectionRadius = 600.0f;
            }
        }
    }
}
#endif