#pragma once

#include "ACFAIController.h"
#include "ACFCoreTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "AIOverseenComponent.generated.h"

UCLASS(ClassGroup = (Portal), meta = (BlueprintSpawnableComponent))
class PORTAL_API UAIOverseenComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UAIOverseenComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Integration with ACF AI Controller
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void InitializeWithController();

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void ReportDeath();

    // ACF Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetCombatTeam(ETeam NewTeam);

    UFUNCTION(BlueprintPure, Category = "AI Overseen")
    bool IsACFCharacter() const;

    // AI Controller Access
    UFUNCTION(BlueprintPure, Category = "AI Overseen")
    AACFAIController* GetACFController() const { return ACFAIController; }

    // Overlord Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetOverlordTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void ApplyOverlordUpgrade(float MovementMultiplier, float DetectionMultiplier, bool bEnableAdvancedTactics);

    // Patrol System Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetPatrolCenter(FVector Center);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetPatrolRadius(float Radius);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void StartPatrolling();

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void StopPatrolling();

    // Portal Defense Integration
    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void SetPortalDefenseMode(bool bDefendPortal);

    UFUNCTION(BlueprintCallable, Category = "AI Overseen")
    void AlertToPlayerPresence(FVector PlayerLocation);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "AI Overseen")
    TObjectPtr<AACFAIController> ACFAIController;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen")
    bool bAutoRegisterWithOverlord = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen")
    bool bIntegrateWithACFTeams = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen")
    ETeam DefaultGuardTeam = ETeam::ETeam2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen")
    bool bAutoSetPatrolBehavior = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen")
    bool bDefendPortal = true;

    // Patrol Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen | Patrol")
    float DefaultPatrolRadius = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen | Patrol")
    bool bUseSpawnLocationAsPatrolCenter = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen | Patrol")
    FVector CustomPatrolCenter = FVector::ZeroVector;

    // Portal Defense Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen | Defense")
    float PlayerDetectionRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen | Defense")
    float MaxChaseDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen | Defense")
    bool bAlertOtherGuards = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Overseen | Defense")
    float AlertRadius = 1500.0f;

private:
    UFUNCTION()
    void OnOwnerDestroyed(AActor* DestroyedActor);

    UFUNCTION()
    void OnACFCharacterDeath();

    void SetupACFIntegration();
    void SetPortalAsDefenseTarget();
    void SetupPatrolBehavior();
};