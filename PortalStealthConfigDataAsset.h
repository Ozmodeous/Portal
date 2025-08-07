#pragma once

#include "ACFStealthDetectionComponent.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PortalStealthConfigDataAsset.generated.h"

UCLASS(BlueprintType, Blueprintable)
class PORTAL_API UPortalStealthConfigDataAsset : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UPortalStealthConfigDataAsset();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth Settings")
    FHybridStealthSettings StealthSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TArray<FName> VegetationTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TArray<FName> GrassTags;

    UFUNCTION(BlueprintPure, Category = "Stealth Config")
    FHybridStealthSettings GetStealthSettings() const { return StealthSettings; }

protected:
    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};