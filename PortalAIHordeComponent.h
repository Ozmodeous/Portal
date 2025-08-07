// PortalAIHordeComponent.h
#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PortalAIHordeComponent.generated.h"

// Forward declarations
class APortalACFAIController;
class UPortalAIHordeManager;

/**
 * Horde role types
 */
UENUM(BlueprintType)
enum class EHordeRole : uint8 {
    None UMETA(DisplayName = "None"),
    Leader UMETA(DisplayName = "Leader"),
    Elite UMETA(DisplayName = "Elite"),
    Soldier UMETA(DisplayName = "Soldier"),
    Scout UMETA(DisplayName = "Scout"),
    Support UMETA(DisplayName = "Support")
};

/**
 * Horde formation types
 */
UENUM(BlueprintType)
enum class EHordeFormation : uint8 {
    None UMETA(DisplayName = "None"),
    Line UMETA(DisplayName = "Line"),
    Wedge UMETA(DisplayName = "Wedge"),
    Circle UMETA(DisplayName = "Circle"),
    Scattered UMETA(DisplayName = "Scattered"),
    Flanking UMETA(DisplayName = "Flanking")
};

/**
 * Horde behavior state
 */
UENUM(BlueprintType)
enum class EHordeState : uint8 {
    Idle UMETA(DisplayName = "Idle"),
    Patrolling UMETA(DisplayName = "Patrolling"),
    Searching UMETA(DisplayName = "Searching"),
    Engaging UMETA(DisplayName = "Engaging"),
    Retreating UMETA(DisplayName = "Retreating"),
    Regrouping UMETA(DisplayName = "Regrouping")
};

/**
 * Horde member data
 */
USTRUCT(BlueprintType)
struct FHordeMemberData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    APortalACFAIController* Controller = nullptr;

    UPROPERTY(BlueprintReadOnly)
    EHordeRole Role = EHordeRole::Soldier;

    UPROPERTY(BlueprintReadOnly)
    FVector AssignedPosition = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float DistanceToTarget = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsInPosition = false;

    UPROPERTY(BlueprintReadOnly)
    float LastCommandTime = 0.0f;

    FHordeMemberData()
    {
        Controller = nullptr;
        Role = EHordeRole::Soldier;
        AssignedPosition = FVector::ZeroVector;
        DistanceToTarget = 0.0f;
        bIsInPosition = false;
        LastCommandTime = 0.0f;
    }
};

/**
 * Horde command structure
 */
USTRUCT(BlueprintType)
struct FHordeCommand {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FName CommandType;

    UPROPERTY(BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    AActor* TargetActor = nullptr;

    UPROPERTY(BlueprintReadWrite)
    EHordeFormation Formation = EHordeFormation::None;

    UPROPERTY(BlueprintReadWrite)
    float Priority = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float Timestamp = 0.0f;

    FHordeCommand()
    {
        CommandType = NAME_None;
        TargetLocation = FVector::ZeroVector;
        TargetActor = nullptr;
        Formation = EHordeFormation::None;
        Priority = 1.0f;
        Timestamp = 0.0f;
    }
};

/**
 * AI Horde Component for Portal AI
 *
 * Manages coordinated group behavior for AI entities
 * Handles formations, group tactics, and synchronized actions
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL_API UPortalAIHordeComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UPortalAIHordeComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    bool bAutoRegister = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    EHordeRole DefaultRole = EHordeRole::Soldier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    float MaxHordeSize = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    float FormationSpacing = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    float CoordinationRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    float CommandResponseTime = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    bool bCanInitiateCommands = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde Config")
    float LeaderInfluenceRadius = 1500.0f;

    // Behavior settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    float FlockingSeparation = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    float FlockingAlignment = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    float FlockingCohesion = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bUseFlocking = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bMaintainFormation = true;

    // State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde State")
    int32 HordeID = -1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde State")
    EHordeRole CurrentRole = EHordeRole::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde State")
    EHordeState CurrentState = EHordeState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde State")
    EHordeFormation CurrentFormation = EHordeFormation::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde State")
    bool bIsRegistered = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horde State")
    FVector AssignedFormationPosition = FVector::ZeroVector;

public:
    // Initialization
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void Initialize(APortalACFAIController* Controller);

    // Registration
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void RegisterWithHorde();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void UnregisterFromHorde();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void JoinHorde(int32 NewHordeID);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void LeaveHorde();

    // Role management
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void SetRole(EHordeRole NewRole);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    EHordeRole GetRole() const { return CurrentRole; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    bool IsLeader() const { return CurrentRole == EHordeRole::Leader; }

    // Behavior updates
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void UpdateHordeBehavior(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void ProcessHordeCommand(const FHordeCommand& Command);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void ExecuteFormation(EHordeFormation Formation, const FVector& CenterPoint);

    // Combat coordination
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void OnEngageTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void OnDisengageTarget();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void CoordinateAttack(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void RequestBackup(const FVector& Location);

    // Formation management
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    FVector CalculateFormationPosition(int32 MemberIndex, int32 TotalMembers, EHordeFormation Formation, const FVector& CenterPoint) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void MoveToFormationPosition();

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    bool IsInFormation() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    float GetDistanceToFormationPosition() const;

    // Communication
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void BroadcastCommand(const FHordeCommand& Command);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void SendCommandToNearby(const FHordeCommand& Command, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    TArray<UPortalAIHordeComponent*> GetNearbyHordeMembers(float Radius) const;

    // State management
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    void SetHordeState(EHordeState NewState);

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    EHordeState GetHordeState() const { return CurrentState; }

    // Flocking behavior
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    FVector CalculateFlockingForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    FVector CalculateSeparationForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    FVector CalculateAlignmentForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    FVector CalculateCohesionForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const;

    // Queries
    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    int32 GetHordeID() const { return HordeID; }

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    int32 GetHordeSize() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    UPortalAIHordeComponent* GetHordeLeader() const;

    UFUNCTION(BlueprintCallable, Category = "Portal AI Horde")
    bool HasTarget() const;

    // Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde")
    void OnJoinedHorde(int32 NewHordeID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde")
    void OnLeftHorde();

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde")
    void OnRoleChanged(EHordeRole NewRole);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde")
    void OnHordeStateChanged(EHordeState NewState);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde")
    void OnCommandReceived(const FHordeCommand& Command);

    UFUNCTION(BlueprintImplementableEvent, Category = "Portal AI Horde")
    void OnFormationChanged(EHordeFormation NewFormation);

private:
    UPROPERTY()
    APortalACFAIController* OwnerController;

    UPROPERTY()
    UPortalAIHordeManager* HordeManager;

    // Internal state
    FHordeCommand LastReceivedCommand;
    float LastUpdateTime = 0.0f;
    TArray<UPortalAIHordeComponent*> CachedNearbyMembers;
    float NearbyMembersCacheTime = 0.0f;
    float NearbyMembersCacheDuration = 0.5f;

    // Target tracking
    UPROPERTY()
    AActor* CurrentTarget;
    float TargetEngageTime = 0.0f;

    // Internal methods
    void UpdateFormationPosition(float DeltaTime);
    void UpdateFlockingBehavior(float DeltaTime);
    void ProcessLeaderBehavior(float DeltaTime);
    void SyncWithHordeManager();
    FVector GetHordeCenterPosition() const;
    void ApplyMovementToController(const FVector& DesiredVelocity);
};