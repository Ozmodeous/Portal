// PortalACFAIController.h
#pragma once

// Include the correct ACF headers
#include "ACFAIController.h" // Corrected include path
#include "CoreMinimal.h"
#include "GameFramework/Controller.h"
#include "PortalACFAIController.generated.h"

// Forward declarations
class UPortalAIDifficultyComponent;
class UPortalAILearningComponent;
class UPortalAIFOVComponent;
class UPortalAIHordeComponent;
class UBlackboardComponent;
class UBehaviorTreeComponent;

/**
 * Portal ACF AI Controller
 *
 * Extended ACF AI Controller that serves as the base for all Portal AI.
 * Uses a component-based architecture for modularity and performance.
 */
UCLASS()
class PORTAL_API APortalACFAIController : public AACFAIController {
    GENERATED_BODY()

public:
    // Use only the FObjectInitializer constructor that ACF requires
    APortalACFAIController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaTime) override;

    // Component references
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal AI Components")
    UPortalAIDifficultyComponent* DifficultyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal AI Components")
    UPortalAILearningComponent* LearningComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal AI Components")
    UPortalAIFOVComponent* FOVComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal AI Components")
    UPortalAIHordeComponent* HordeComponent;

    // Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal AI Config")
    bool bEnableLearning = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal AI Config")
    bool bEnableHordeBehavior = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal AI Config")
    float BaseReactionTime = 0.5f;

public:
    // Component access
    UFUNCTION(BlueprintCallable, Category = "Portal AI")
    UPortalAIDifficultyComponent* GetDifficultyComponent() const { return DifficultyComponent; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI")
    UPortalAILearningComponent* GetLearningComponent() const { return LearningComponent; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI")
    UPortalAIFOVComponent* GetFOVComponent() const { return FOVComponent; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI")
    UPortalAIHordeComponent* GetHordeComponent() const { return HordeComponent; }

    // AI State Management
    UFUNCTION(BlueprintCallable, Category = "Portal AI")
    void UpdateAIState();

    UFUNCTION(BlueprintCallable, Category = "Portal AI")
    void SetDifficulty(float NewDifficulty);

    // Event handlers
    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI")
    void OnAIStateChanged();

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI")
    void OnTargetAcquired(AActor* Target);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI")
    void OnTargetLost();

private:
    // Internal state
    float CurrentDifficulty = 1.0f;
    bool bIsInCombat = false;

    // Performance optimization
    float LastComponentUpdateTime = 0.0f;
    float ComponentUpdateInterval = 0.1f; // Update components every 100ms

    void UpdateComponents(float DeltaTime);
    void InitializeComponents();
};