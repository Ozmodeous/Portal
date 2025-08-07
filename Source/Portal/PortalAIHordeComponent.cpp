// PortalAIHordeComponent.cpp
#include "PortalAIHordeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "PortalACFAIController.h"
#include "PortalAIHordeManager.h"

UPortalAIHordeComponent::UPortalAIHordeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Set default configuration
    bAutoRegister = true;
    DefaultRole = EHordeRole::Soldier;
    MaxHordeSize = 20.0f;
    FormationSpacing = 200.0f;
    CoordinationRadius = 1000.0f;
    CommandResponseTime = 0.5f;
    bCanInitiateCommands = false;
    LeaderInfluenceRadius = 1500.0f;

    // Flocking parameters
    FlockingSeparation = 100.0f;
    FlockingAlignment = 0.5f;
    FlockingCohesion = 0.3f;
    bUseFlocking = true;
    bMaintainFormation = true;
}

void UPortalAIHordeComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRegister) {
        RegisterWithHorde();
    }
}

void UPortalAIHordeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromHorde();
    Super::EndPlay(EndPlayReason);
}

void UPortalAIHordeComponent::Initialize(APortalACFAIController* Controller)
{
    OwnerController = Controller;

    if (!OwnerController) {
        UE_LOG(LogTemp, Warning, TEXT("PortalAIHordeComponent: No valid controller provided"));
        return;
    }

    // Get horde manager
    HordeManager = UPortalAIHordeManager::GetHordeManager(GetWorld());

    UE_LOG(LogTemp, Log, TEXT("Horde Component initialized"));
}

void UPortalAIHordeComponent::RegisterWithHorde()
{
    if (!HordeManager) {
        HordeManager = UPortalAIHordeManager::GetHordeManager(GetWorld());
    }

    if (HordeManager && !bIsRegistered) {
        bIsRegistered = HordeManager->RegisterAI(this);

        if (bIsRegistered) {
            SetRole(DefaultRole);
            UE_LOG(LogTemp, Log, TEXT("AI registered with horde system"));
        }
    }
}

void UPortalAIHordeComponent::UnregisterFromHorde()
{
    if (HordeManager && bIsRegistered) {
        HordeManager->UnregisterAI(this);
        bIsRegistered = false;
        HordeID = -1;
        CurrentRole = EHordeRole::None;

        UE_LOG(LogTemp, Log, TEXT("AI unregistered from horde system"));
    }
}

void UPortalAIHordeComponent::JoinHorde(int32 NewHordeID)
{
    if (!HordeManager) {
        return;
    }

    // Leave current horde if in one
    if (HordeID != -1) {
        LeaveHorde();
    }

    // Join new horde
    if (HordeManager->AddToHorde(this, NewHordeID)) {
        HordeID = NewHordeID;
        OnJoinedHorde(HordeID);

        UE_LOG(LogTemp, Log, TEXT("Joined horde %d"), HordeID);
    }
}

void UPortalAIHordeComponent::LeaveHorde()
{
    if (HordeManager && HordeID != -1) {
        HordeManager->RemoveFromHorde(this, HordeID);
        int32 OldHordeID = HordeID;
        HordeID = -1;
        CurrentRole = EHordeRole::None;
        CurrentFormation = EHordeFormation::None;

        OnLeftHorde();

        UE_LOG(LogTemp, Log, TEXT("Left horde %d"), OldHordeID);
    }
}

void UPortalAIHordeComponent::SetRole(EHordeRole NewRole)
{
    if (CurrentRole != NewRole) {
        CurrentRole = NewRole;

        // Update behavior based on role
        if (OwnerController && OwnerController->GetBlackboardComponent()) {
            OwnerController->GetBlackboardComponent()->SetValueAsEnum("HordeRole", (uint8)CurrentRole);
        }

        // Update command authority
        bCanInitiateCommands = (CurrentRole == EHordeRole::Leader || CurrentRole == EHordeRole::Elite);

        OnRoleChanged(CurrentRole);

        UE_LOG(LogTemp, Log, TEXT("Horde role changed to %d"), (int32)CurrentRole);
    }
}

void UPortalAIHordeComponent::UpdateHordeBehavior(float DeltaTime)
{
    if (!bIsRegistered || HordeID == -1 || !OwnerController) {
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();

    // Throttle updates
    if (CurrentTime - LastUpdateTime < 0.1f) {
        return;
    }

    LastUpdateTime = CurrentTime;

    // Sync with horde manager
    SyncWithHordeManager();

    // Update based on role
    if (IsLeader()) {
        ProcessLeaderBehavior(DeltaTime);
    }

    // Update formation position if needed
    if (bMaintainFormation && CurrentFormation != EHordeFormation::None) {
        UpdateFormationPosition(DeltaTime);
    }

    // Apply flocking behavior if enabled
    if (bUseFlocking) {
        UpdateFlockingBehavior(DeltaTime);
    }
}

void UPortalAIHordeComponent::ProcessHordeCommand(const FHordeCommand& Command)
{
    // Check if command is newer than last processed
    if (Command.Timestamp <= LastReceivedCommand.Timestamp) {
        return;
    }

    LastReceivedCommand = Command;
    float ResponseDelay = CommandResponseTime;

    // Leaders respond immediately
    if (IsLeader()) {
        ResponseDelay = 0.0f;
    }

    // Execute command after delay
    FTimerHandle CommandTimer;
    GetWorld()->GetTimerManager().SetTimer(CommandTimer, [this, Command]() {
        // Process command based on type
        if (Command.CommandType == "Attack")
        {
            if (Command.TargetActor)
            {
                OnEngageTarget(Command.TargetActor);
            }
        }
        else if (Command.CommandType == "Move")
        {
            MoveToFormationPosition();
        }
        else if (Command.CommandType == "Formation")
        {
            CurrentFormation = Command.Formation;
            OnFormationChanged(CurrentFormation);
        }
        else if (Command.CommandType == "Retreat")
        {
            SetHordeState(EHordeState::Retreating);
        } }, ResponseDelay, false);

    OnCommandReceived(Command);
}

void UPortalAIHordeComponent::ExecuteFormation(EHordeFormation Formation, const FVector& CenterPoint)
{
    CurrentFormation = Formation;

    if (HordeManager && HordeID != -1) {
        // Get our position in formation
        TArray<UPortalAIHordeComponent*> HordeMembers = HordeManager->GetHordeMembers(HordeID);
        int32 MyIndex = HordeMembers.Find(this);

        if (MyIndex != INDEX_NONE) {
            AssignedFormationPosition = CalculateFormationPosition(
                MyIndex,
                HordeMembers.Num(),
                Formation,
                CenterPoint);

            MoveToFormationPosition();
        }
    }

    OnFormationChanged(Formation);
}

void UPortalAIHordeComponent::OnEngageTarget(AActor* Target)
{
    if (!Target) {
        return;
    }

    CurrentTarget = Target;
    TargetEngageTime = GetWorld()->GetTimeSeconds();
    SetHordeState(EHordeState::Engaging);

    // Update blackboard
    if (OwnerController && OwnerController->GetBlackboardComponent()) {
        OwnerController->GetBlackboardComponent()->SetValueAsObject("HordeTarget", Target);
    }

    // If we're a leader, coordinate the attack
    if (IsLeader()) {
        CoordinateAttack(Target);
    }
}

void UPortalAIHordeComponent::OnDisengageTarget()
{
    CurrentTarget = nullptr;
    SetHordeState(EHordeState::Idle);

    // Clear blackboard
    if (OwnerController && OwnerController->GetBlackboardComponent()) {
        OwnerController->GetBlackboardComponent()->ClearValue("HordeTarget");
    }
}

void UPortalAIHordeComponent::CoordinateAttack(AActor* Target)
{
    if (!Target || !IsLeader()) {
        return;
    }

    // Create attack command
    FHordeCommand AttackCommand;
    AttackCommand.CommandType = "Attack";
    AttackCommand.TargetActor = Target;
    AttackCommand.Formation = EHordeFormation::Flanking;
    AttackCommand.Priority = 1.0f;
    AttackCommand.Timestamp = GetWorld()->GetTimeSeconds();

    // Broadcast to horde
    BroadcastCommand(AttackCommand);

    UE_LOG(LogTemp, Log, TEXT("Leader coordinating attack on %s"), *Target->GetName());
}

void UPortalAIHordeComponent::RequestBackup(const FVector& Location)
{
    if (!bCanInitiateCommands) {
        return;
    }

    // Create reinforcement command
    FHordeCommand BackupCommand;
    BackupCommand.CommandType = "Reinforce";
    BackupCommand.TargetLocation = Location;
    BackupCommand.Priority = 2.0f;
    BackupCommand.Timestamp = GetWorld()->GetTimeSeconds();

    // Send to nearby hordes
    SendCommandToNearby(BackupCommand, CoordinationRadius * 2.0f);

    UE_LOG(LogTemp, Log, TEXT("Backup requested at location %s"), *Location.ToString());
}

FVector UPortalAIHordeComponent::CalculateFormationPosition(int32 MemberIndex, int32 TotalMembers,
    EHordeFormation Formation,
    const FVector& CenterPoint) const
{
    FVector Position = CenterPoint;

    switch (Formation) {
    case EHordeFormation::Line: {
        // Horizontal line formation
        float Offset = (MemberIndex - TotalMembers / 2) * FormationSpacing;
        Position += FVector(0, Offset, 0);
    } break;

    case EHordeFormation::Wedge: {
        // V-shaped formation
        int32 Row = 0;
        int32 ColInRow = MemberIndex;
        int32 MembersInRow = 1;

        while (ColInRow >= MembersInRow) {
            ColInRow -= MembersInRow;
            Row++;
            MembersInRow++;
        }

        float RowOffset = Row * FormationSpacing;
        float ColOffset = (ColInRow - MembersInRow / 2.0f) * FormationSpacing;

        Position += FVector(-RowOffset, ColOffset, 0);
    } break;

    case EHordeFormation::Circle: {
        // Circular formation
        float Angle = (2.0f * PI * MemberIndex) / TotalMembers;
        float Radius = FormationSpacing * TotalMembers / (2.0f * PI);

        Position += FVector(
            FMath::Cos(Angle) * Radius,
            FMath::Sin(Angle) * Radius,
            0);
    } break;

    case EHordeFormation::Scattered: {
        // Random scattered positions
        FRandomStream RandomStream(MemberIndex);
        float RandomAngle = RandomStream.FRand() * 2.0f * PI;
        float RandomDistance = RandomStream.FRand() * FormationSpacing * 3.0f;

        Position += FVector(
            FMath::Cos(RandomAngle) * RandomDistance,
            FMath::Sin(RandomAngle) * RandomDistance,
            0);
    } break;

    case EHordeFormation::Flanking: {
        // Two groups flanking from sides
        bool bLeftFlank = (MemberIndex % 2 == 0);
        int32 GroupIndex = MemberIndex / 2;

        float ForwardOffset = GroupIndex * FormationSpacing;
        float SideOffset = (bLeftFlank ? -1.0f : 1.0f) * (FormationSpacing * 2.0f);

        Position += FVector(ForwardOffset, SideOffset, 0);
    } break;

    default:
        break;
    }

    return Position;
}

void UPortalAIHordeComponent::MoveToFormationPosition()
{
    if (!OwnerController || !OwnerController->GetPawn()) {
        return;
    }

    APawn* Pawn = OwnerController->GetPawn();

    // Use navigation system to find valid position
    UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSystem) {
        return;
    }

    FNavLocation NavLocation;
    if (NavSystem->GetRandomReachablePointInRadius(AssignedFormationPosition, 50.0f, NavLocation)) {
        OwnerController->MoveToLocation(NavLocation.Location, 50.0f, true, true, false, true);

        // Update blackboard
        if (OwnerController->GetBlackboardComponent()) {
            OwnerController->GetBlackboardComponent()->SetValueAsVector("FormationPosition", NavLocation.Location);
        }
    }
}

bool UPortalAIHordeComponent::IsInFormation() const
{
    if (!OwnerController || !OwnerController->GetPawn()) {
        return false;
    }

    float Distance = GetDistanceToFormationPosition();
    return Distance < FormationSpacing * 0.5f;
}

float UPortalAIHordeComponent::GetDistanceToFormationPosition() const
{
    if (!OwnerController || !OwnerController->GetPawn()) {
        return MAX_FLT;
    }

    return FVector::Dist(OwnerController->GetPawn()->GetActorLocation(), AssignedFormationPosition);
}

void UPortalAIHordeComponent::BroadcastCommand(const FHordeCommand& Command)
{
    if (!HordeManager || HordeID == -1) {
        return;
    }

    HordeManager->BroadcastCommandToHorde(HordeID, Command);
}

void UPortalAIHordeComponent::SendCommandToNearby(const FHordeCommand& Command, float Radius)
{
    TArray<UPortalAIHordeComponent*> NearbyMembers = GetNearbyHordeMembers(Radius);

    for (UPortalAIHordeComponent* Member : NearbyMembers) {
        if (Member && Member != this) {
            Member->ProcessHordeCommand(Command);
        }
    }
}

TArray<UPortalAIHordeComponent*> UPortalAIHordeComponent::GetNearbyHordeMembers(float Radius) const
{
    // Use cached results if recent enough
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - NearbyMembersCacheTime < NearbyMembersCacheDuration) {
        return CachedNearbyMembers;
    }

    // Update cache - need to cast away const to modify cache
    UPortalAIHordeComponent* NonConstThis = const_cast<UPortalAIHordeComponent*>(this);
    NonConstThis->CachedNearbyMembers.Empty();

    if (!OwnerController || !OwnerController->GetPawn()) {
        NonConstThis->NearbyMembersCacheTime = CurrentTime;
        return CachedNearbyMembers;
    }

    FVector MyLocation = OwnerController->GetPawn()->GetActorLocation();

    // Get all actors with horde component
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors) {
        if (APawn* Pawn = Cast<APawn>(Actor)) {
            if (FVector::Dist(MyLocation, Pawn->GetActorLocation()) <= Radius) {
                if (APortalACFAIController* AIController = Cast<APortalACFAIController>(Pawn->GetController())) {
                    if (UPortalAIHordeComponent* HordeComp = AIController->GetHordeComponent()) {
                        if (HordeComp != this && HordeComp->HordeID == HordeID) {
                            NonConstThis->CachedNearbyMembers.Add(HordeComp);
                        }
                    }
                }
            }
        }
    }

    NonConstThis->NearbyMembersCacheTime = CurrentTime;
    return CachedNearbyMembers;
}

void UPortalAIHordeComponent::SetHordeState(EHordeState NewState)
{
    if (CurrentState != NewState) {
        CurrentState = NewState;

        // Update blackboard
        if (OwnerController && OwnerController->GetBlackboardComponent()) {
            OwnerController->GetBlackboardComponent()->SetValueAsEnum("HordeState", (uint8)CurrentState);
        }

        OnHordeStateChanged(CurrentState);

        UE_LOG(LogTemp, Verbose, TEXT("Horde state changed to %d"), (int32)CurrentState);
    }
}

FVector UPortalAIHordeComponent::CalculateFlockingForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const
{
    if (!bUseFlocking || NearbyMembers.Num() == 0) {
        return FVector::ZeroVector;
    }

    FVector SeparationForce = CalculateSeparationForce(NearbyMembers);
    FVector AlignmentForce = CalculateAlignmentForce(NearbyMembers);
    FVector CohesionForce = CalculateCohesionForce(NearbyMembers);

    // Combine forces with weights
    FVector TotalForce = SeparationForce + (AlignmentForce * FlockingAlignment) + (CohesionForce * FlockingCohesion);

    return TotalForce.GetClampedToMaxSize(1.0f);
}

FVector UPortalAIHordeComponent::CalculateSeparationForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const
{
    if (!OwnerController || !OwnerController->GetPawn()) {
        return FVector::ZeroVector;
    }

    FVector SeparationForce = FVector::ZeroVector;
    FVector MyLocation = OwnerController->GetPawn()->GetActorLocation();
    int32 NeighborCount = 0;

    for (const UPortalAIHordeComponent* Member : NearbyMembers) {
        if (!Member || !Member->OwnerController || !Member->OwnerController->GetPawn()) {
            continue;
        }

        FVector MemberLocation = Member->OwnerController->GetPawn()->GetActorLocation();
        float Distance = FVector::Dist(MyLocation, MemberLocation);

        if (Distance < FlockingSeparation && Distance > 0.01f) {
            FVector AwayVector = (MyLocation - MemberLocation).GetSafeNormal();
            float Strength = 1.0f - (Distance / FlockingSeparation);
            SeparationForce += AwayVector * Strength;
            NeighborCount++;
        }
    }

    if (NeighborCount > 0) {
        SeparationForce /= NeighborCount;
    }

    return SeparationForce;
}

FVector UPortalAIHordeComponent::CalculateAlignmentForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const
{
    if (!OwnerController || !OwnerController->GetPawn()) {
        return FVector::ZeroVector;
    }

    FVector AverageVelocity = FVector::ZeroVector;
    int32 NeighborCount = 0;

    for (const UPortalAIHordeComponent* Member : NearbyMembers) {
        if (!Member || !Member->OwnerController || !Member->OwnerController->GetPawn()) {
            continue;
        }

        if (ACharacter* Character = Cast<ACharacter>(Member->OwnerController->GetPawn())) {
            if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement()) {
                AverageVelocity += MoveComp->Velocity;
                NeighborCount++;
            }
        }
    }

    if (NeighborCount > 0) {
        AverageVelocity /= NeighborCount;
        return AverageVelocity.GetSafeNormal();
    }

    return FVector::ZeroVector;
}

FVector UPortalAIHordeComponent::CalculateCohesionForce(const TArray<UPortalAIHordeComponent*>& NearbyMembers) const
{
    if (!OwnerController || !OwnerController->GetPawn()) {
        return FVector::ZeroVector;
    }

    FVector CenterOfMass = FVector::ZeroVector;
    int32 NeighborCount = 0;

    for (const UPortalAIHordeComponent* Member : NearbyMembers) {
        if (!Member || !Member->OwnerController || !Member->OwnerController->GetPawn()) {
            continue;
        }

        CenterOfMass += Member->OwnerController->GetPawn()->GetActorLocation();
        NeighborCount++;
    }

    if (NeighborCount > 0) {
        CenterOfMass /= NeighborCount;
        FVector MyLocation = OwnerController->GetPawn()->GetActorLocation();
        return (CenterOfMass - MyLocation).GetSafeNormal();
    }

    return FVector::ZeroVector;
}

int32 UPortalAIHordeComponent::GetHordeSize() const
{
    if (!HordeManager || HordeID == -1) {
        return 0;
    }

    return HordeManager->GetHordeSize(HordeID);
}

UPortalAIHordeComponent* UPortalAIHordeComponent::GetHordeLeader() const
{
    if (!HordeManager || HordeID == -1) {
        return nullptr;
    }

    return HordeManager->GetHordeLeader(HordeID);
}

bool UPortalAIHordeComponent::HasTarget() const
{
    return CurrentTarget != nullptr;
}

void UPortalAIHordeComponent::UpdateFormationPosition(float DeltaTime)
{
    if (!IsInFormation()) {
        MoveToFormationPosition();
    }
}

void UPortalAIHordeComponent::UpdateFlockingBehavior(float DeltaTime)
{
    TArray<UPortalAIHordeComponent*> NearbyMembers = GetNearbyHordeMembers(FlockingSeparation * 3.0f);

    if (NearbyMembers.Num() > 0) {
        FVector FlockingForce = CalculateFlockingForce(NearbyMembers);
        ApplyMovementToController(FlockingForce * 100.0f); // Scale force appropriately
    }
}

void UPortalAIHordeComponent::ProcessLeaderBehavior(float DeltaTime)
{
    // Leader-specific behavior
    if (!IsLeader()) {
        return;
    }

    // Check if formation needs adjustment
    if (CurrentFormation != EHordeFormation::None) {
        TArray<UPortalAIHordeComponent*> HordeMembers = GetNearbyHordeMembers(CoordinationRadius);

        int32 OutOfFormationCount = 0;
        for (const UPortalAIHordeComponent* Member : HordeMembers) {
            if (!Member->IsInFormation()) {
                OutOfFormationCount++;
            }
        }

        // Reissue formation command if too many are out of position
        if (OutOfFormationCount > HordeMembers.Num() / 3) {
            FVector HordeCenter = GetHordeCenterPosition();
            ExecuteFormation(CurrentFormation, HordeCenter);
        }
    }

    // Check if we need to change state
    if (CurrentState == EHordeState::Engaging && !HasTarget()) {
        SetHordeState(EHordeState::Searching);
    }
}

void UPortalAIHordeComponent::SyncWithHordeManager()
{
    if (!HordeManager || HordeID == -1) {
        return;
    }

    // Get current horde info from manager
    FHordeGroup HordeInfo = HordeManager->GetHordeInfo(HordeID);

    // Sync state
    if (HordeInfo.CurrentState != CurrentState) {
        SetHordeState(HordeInfo.CurrentState);
    }

    // Sync formation
    if (HordeInfo.CurrentFormation != CurrentFormation) {
        CurrentFormation = HordeInfo.CurrentFormation;
        OnFormationChanged(CurrentFormation);
    }

    // Sync target
    if (HordeInfo.PrimaryTarget != CurrentTarget) {
        if (HordeInfo.PrimaryTarget) {
            OnEngageTarget(HordeInfo.PrimaryTarget);
        } else {
            OnDisengageTarget();
        }
    }
}

FVector UPortalAIHordeComponent::GetHordeCenterPosition() const
{
    if (!HordeManager || HordeID == -1) {
        return FVector::ZeroVector;
    }

    return HordeManager->GetHordeCenter(HordeID);
}

void UPortalAIHordeComponent::ApplyMovementToController(const FVector& DesiredVelocity)
{
    if (!OwnerController || !OwnerController->GetPawn()) {
        return;
    }

    if (ACharacter* Character = Cast<ACharacter>(OwnerController->GetPawn())) {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement()) {
            // Add flocking force as additional velocity
            MoveComp->AddInputVector(DesiredVelocity.GetSafeNormal());
        }
    }
}