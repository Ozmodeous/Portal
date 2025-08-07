// PortalAIHordeManager.cpp
#include "PortalAIHordeManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "PortalACFAIController.h"
#include "PortalAIHordeComponent.h"
#include "TimerManager.h"

void UPortalAIHordeManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Initialize manager
    NextHordeID = 1;
    TotalActiveAI = 0;

    // Start update timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(UpdateTimerHandle, this,
            &UPortalAIHordeManager::UpdateHordes, UpdateInterval, true);
    }

    UE_LOG(LogTemp, Log, TEXT("Portal AI Horde Manager initialized"));
}

void UPortalAIHordeManager::Deinitialize()
{
    // Clean up all hordes
    TArray<int32> HordeIDs;
    ActiveHordes.GetKeys(HordeIDs);

    for (int32 HordeID : HordeIDs) {
        DestroyHorde(HordeID);
    }

    // Clear timer
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }

    Super::Deinitialize();

    UE_LOG(LogTemp, Log, TEXT("Portal AI Horde Manager deinitialized"));
}

UPortalAIHordeManager* UPortalAIHordeManager::GetHordeManager(const UObject* WorldContextObject)
{
    if (!WorldContextObject) {
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance) {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UPortalAIHordeManager>();
}

int32 UPortalAIHordeManager::CreateHorde(const TArray<UPortalAIHordeComponent*>& InitialMembers)
{
    if (ActiveHordes.Num() >= MaxActiveHordes) {
        UE_LOG(LogTemp, Warning, TEXT("Cannot create horde: Maximum active hordes reached"));
        return -1;
    }

    int32 NewHordeID = GenerateHordeID();

    FHordeGroup NewHorde;
    NewHorde.HordeID = NewHordeID;
    NewHorde.Members = InitialMembers;
    NewHorde.CurrentState = EHordeState::Idle;
    NewHorde.CurrentFormation = EHordeFormation::None;
    NewHorde.LastUpdateTime = GetWorld()->GetTimeSeconds();

    // Set all members to this horde
    for (UPortalAIHordeComponent* Member : InitialMembers) {
        if (Member) {
            Member->JoinHorde(NewHordeID);
        }
    }

    // Auto-assign leader
    AutoAssignLeader(NewHordeID);

    // Calculate initial center
    RecalculateHordeCenter(NewHordeID);

    ActiveHordes.Add(NewHordeID, NewHorde);
    TotalActiveAI += InitialMembers.Num();

    OnHordeCreated(NewHordeID);

    UE_LOG(LogTemp, Log, TEXT("Created horde %d with %d members"), NewHordeID, InitialMembers.Num());

    return NewHordeID;
}

bool UPortalAIHordeManager::DestroyHorde(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return false;
    }

    // Remove all members from horde
    for (UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member) {
            Member->LeaveHorde();
        }
    }

    TotalActiveAI -= Horde->Members.Num();
    ActiveHordes.Remove(HordeID);

    OnHordeDestroyed(HordeID);

    UE_LOG(LogTemp, Log, TEXT("Destroyed horde %d"), HordeID);

    return true;
}

bool UPortalAIHordeManager::MergeHordes(int32 HordeID1, int32 HordeID2)
{
    FHordeGroup* Horde1 = ActiveHordes.Find(HordeID1);
    FHordeGroup* Horde2 = ActiveHordes.Find(HordeID2);

    if (!Horde1 || !Horde2) {
        return false;
    }

    // Merge smaller horde into larger
    int32 KeepHordeID = Horde1->Members.Num() >= Horde2->Members.Num() ? HordeID1 : HordeID2;
    int32 MergeHordeID = KeepHordeID == HordeID1 ? HordeID2 : HordeID1;

    FHordeGroup* KeepHorde = ActiveHordes.Find(KeepHordeID);
    FHordeGroup* MergeHorde = ActiveHordes.Find(MergeHordeID);

    // Transfer all members
    for (UPortalAIHordeComponent* Member : MergeHorde->Members) {
        if (Member) {
            Member->JoinHorde(KeepHordeID);
            KeepHorde->Members.Add(Member);
        }
    }

    // Remove the merged horde
    ActiveHordes.Remove(MergeHordeID);

    // Reassign leader if needed
    AutoAssignLeader(KeepHordeID);

    OnHordesMerged(KeepHordeID);

    UE_LOG(LogTemp, Log, TEXT("Merged horde %d into horde %d"), MergeHordeID, KeepHordeID);

    return true;
}

TArray<int32> UPortalAIHordeManager::SplitHorde(int32 HordeID, int32 NumGroups)
{
    TArray<int32> NewHordeIDs;

    FHordeGroup* OriginalHorde = ActiveHordes.Find(HordeID);
    if (!OriginalHorde || NumGroups < 2) {
        return NewHordeIDs;
    }

    int32 MembersPerGroup = FMath::Max(1, OriginalHorde->Members.Num() / NumGroups);
    TArray<TArray<UPortalAIHordeComponent*>> SplitGroups;

    // Divide members into groups
    for (int32 i = 0; i < NumGroups; i++) {
        SplitGroups.Add(TArray<UPortalAIHordeComponent*>());
    }

    for (int32 i = 0; i < OriginalHorde->Members.Num(); i++) {
        int32 GroupIndex = i / MembersPerGroup;
        if (GroupIndex >= NumGroups) {
            GroupIndex = NumGroups - 1;
        }

        SplitGroups[GroupIndex].Add(OriginalHorde->Members[i]);
    }

    // Destroy original horde
    DestroyHorde(HordeID);

    // Create new hordes
    for (const TArray<UPortalAIHordeComponent*>& Group : SplitGroups) {
        if (Group.Num() > 0) {
            int32 NewHordeID = CreateHorde(Group);
            if (NewHordeID != -1) {
                NewHordeIDs.Add(NewHordeID);
            }
        }
    }

    OnHordeSplit(NewHordeIDs);

    UE_LOG(LogTemp, Log, TEXT("Split horde %d into %d new hordes"), HordeID, NewHordeIDs.Num());

    return NewHordeIDs;
}

bool UPortalAIHordeManager::RegisterAI(UPortalAIHordeComponent* AIComponent)
{
    if (!AIComponent || TotalActiveAI >= MaxTotalAI) {
        return false;
    }

    // Find smallest horde or create new one
    int32 SmallestHordeID = -1;
    int32 SmallestSize = INT32_MAX;

    for (const auto& Pair : ActiveHordes) {
        if (Pair.Value.Members.Num() < SmallestSize) {
            SmallestSize = Pair.Value.Members.Num();
            SmallestHordeID = Pair.Key;
        }
    }

    // Create new horde if none exist or all are full
    if (SmallestHordeID == -1 || SmallestSize >= MaxActiveHordes) {
        TArray<UPortalAIHordeComponent*> InitialMembers;
        InitialMembers.Add(AIComponent);
        SmallestHordeID = CreateHorde(InitialMembers);
    } else {
        AddToHorde(AIComponent, SmallestHordeID);
    }

    return SmallestHordeID != -1;
}

bool UPortalAIHordeManager::UnregisterAI(UPortalAIHordeComponent* AIComponent)
{
    if (!AIComponent) {
        return false;
    }

    // Find and remove from any horde
    for (auto& Pair : ActiveHordes) {
        if (Pair.Value.Members.Remove(AIComponent) > 0) {
            TotalActiveAI--;

            // Destroy horde if empty
            if (Pair.Value.Members.Num() == 0) {
                DestroyHorde(Pair.Key);
            } else {
                // Reassign leader if needed
                if (Pair.Value.Leader == AIComponent) {
                    AutoAssignLeader(Pair.Key);
                }
            }

            return true;
        }
    }

    return false;
}

bool UPortalAIHordeManager::AddToHorde(UPortalAIHordeComponent* AIComponent, int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || !AIComponent) {
        return false;
    }

    // Check if already in horde
    if (Horde->Members.Contains(AIComponent)) {
        return true;
    }

    // Add to horde
    Horde->Members.Add(AIComponent);
    AIComponent->JoinHorde(HordeID);
    TotalActiveAI++;

    // Assign role based on current composition
    if (Horde->Members.Num() == 1) {
        AIComponent->SetRole(EHordeRole::Leader);
        Horde->Leader = AIComponent;
    } else if (Horde->Members.Num() <= 5) {
        AIComponent->SetRole(EHordeRole::Elite);
    } else {
        AIComponent->SetRole(EHordeRole::Soldier);
    }

    return true;
}

bool UPortalAIHordeManager::RemoveFromHorde(UPortalAIHordeComponent* AIComponent, int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || !AIComponent) {
        return false;
    }

    if (Horde->Members.Remove(AIComponent) > 0) {
        TotalActiveAI--;

        // Handle leader removal
        if (Horde->Leader == AIComponent) {
            Horde->Leader = nullptr;
            AutoAssignLeader(HordeID);
        }

        // Destroy horde if empty
        if (Horde->Members.Num() == 0) {
            DestroyHorde(HordeID);
        }

        return true;
    }

    return false;
}

bool UPortalAIHordeManager::TransferMember(UPortalAIHordeComponent* AIComponent, int32 FromHordeID, int32 ToHordeID)
{
    if (RemoveFromHorde(AIComponent, FromHordeID)) {
        return AddToHorde(AIComponent, ToHordeID);
    }

    return false;
}

bool UPortalAIHordeManager::AssignLeader(int32 HordeID, UPortalAIHordeComponent* Leader)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || !Leader || !Horde->Members.Contains(Leader)) {
        return false;
    }

    // Remove old leader role
    if (Horde->Leader) {
        Horde->Leader->SetRole(EHordeRole::Elite);
    }

    // Assign new leader
    Horde->Leader = Leader;
    Leader->SetRole(EHordeRole::Leader);

    UE_LOG(LogTemp, Log, TEXT("Assigned new leader to horde %d"), HordeID);

    return true;
}

UPortalAIHordeComponent* UPortalAIHordeManager::GetHordeLeader(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    return Horde ? Horde->Leader : nullptr;
}

void UPortalAIHordeManager::AutoAssignLeader(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || Horde->Members.Num() == 0) {
        return;
    }

    // Find best candidate for leader
    UPortalAIHordeComponent* BestCandidate = nullptr;
    float BestScore = -1.0f;

    for (UPortalAIHordeComponent* Member : Horde->Members) {
        if (!Member) {
            continue;
        }

        float Score = 1.0f;

        // Prefer elites
        if (Member->GetRole() == EHordeRole::Elite) {
            Score += 2.0f;
        }

        // Prefer those who can initiate commands
        if (Member->bCanInitiateCommands) {
            Score += 1.0f;
        }

        if (Score > BestScore) {
            BestScore = Score;
            BestCandidate = Member;
        }
    }

    if (BestCandidate) {
        AssignLeader(HordeID, BestCandidate);
    }
}

void UPortalAIHordeManager::SetHordeFormation(int32 HordeID, EHordeFormation Formation)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    Horde->CurrentFormation = Formation;
    AssignFormationPositions(HordeID);

    // Notify all members
    for (UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member) {
            Member->ExecuteFormation(Formation, Horde->HordeCenter);
        }
    }
}

void UPortalAIHordeManager::MoveHordeTo(int32 HordeID, const FVector& TargetLocation)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    // Create move command
    FHordeCommand MoveCommand;
    MoveCommand.CommandType = "Move";
    MoveCommand.TargetLocation = TargetLocation;
    MoveCommand.Formation = Horde->CurrentFormation;
    MoveCommand.Priority = 1.0f;
    MoveCommand.Timestamp = GetWorld()->GetTimeSeconds();

    BroadcastCommandToHorde(HordeID, MoveCommand);
}

void UPortalAIHordeManager::UpdateFormationPositions(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || Horde->CurrentFormation == EHordeFormation::None) {
        return;
    }

    AssignFormationPositions(HordeID);
}

void UPortalAIHordeManager::EngageTarget(int32 HordeID, AActor* Target)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || !Target) {
        return;
    }

    Horde->PrimaryTarget = Target;
    Horde->CurrentState = EHordeState::Engaging;

    // Create engage command
    FHordeCommand EngageCommand;
    EngageCommand.CommandType = "Attack";
    EngageCommand.TargetActor = Target;
    EngageCommand.Formation = EHordeFormation::Flanking;
    EngageCommand.Priority = 2.0f;
    EngageCommand.Timestamp = GetWorld()->GetTimeSeconds();

    BroadcastCommandToHorde(HordeID, EngageCommand);

    OnHordeEngaged(HordeID, Target);
}

void UPortalAIHordeManager::DisengageHorde(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    Horde->PrimaryTarget = nullptr;
    Horde->CurrentState = EHordeState::Idle;

    // Create disengage command
    FHordeCommand DisengageCommand;
    DisengageCommand.CommandType = "Disengage";
    DisengageCommand.Priority = 2.0f;
    DisengageCommand.Timestamp = GetWorld()->GetTimeSeconds();

    BroadcastCommandToHorde(HordeID, DisengageCommand);
}

void UPortalAIHordeManager::CoordinateMultiHordeAttack(const TArray<int32>& HordeIDs, AActor* Target)
{
    if (!Target || HordeIDs.Num() == 0) {
        return;
    }

    // Assign different attack patterns to each horde
    TArray<EHordeFormation> AttackFormations = {
        EHordeFormation::Flanking,
        EHordeFormation::Wedge,
        EHordeFormation::Circle
    };

    for (int32 i = 0; i < HordeIDs.Num(); i++) {
        FHordeGroup* Horde = ActiveHordes.Find(HordeIDs[i]);
        if (Horde) {
            // Assign formation based on index
            EHordeFormation Formation = AttackFormations[i % AttackFormations.Num()];

            // Stagger attack timing
            float Delay = i * 0.5f;

            FTimerHandle AttackTimer;
            GetWorld()->GetTimerManager().SetTimer(AttackTimer, [this, HordeIDs, i, Target, Formation]() {
                if (IsValid(Target))
                {
                    SetHordeFormation(HordeIDs[i], Formation);
                    EngageTarget(HordeIDs[i], Target);
                } }, Delay, false);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Coordinated multi-horde attack with %d hordes"), HordeIDs.Num());
}

void UPortalAIHordeManager::InitiateRetreat(int32 HordeID, const FVector& RetreatLocation)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    Horde->CurrentState = EHordeState::Retreating;

    // Create retreat command
    FHordeCommand RetreatCommand;
    RetreatCommand.CommandType = "Retreat";
    RetreatCommand.TargetLocation = RetreatLocation;
    RetreatCommand.Formation = EHordeFormation::Scattered;
    RetreatCommand.Priority = 3.0f;
    RetreatCommand.Timestamp = GetWorld()->GetTimeSeconds();

    BroadcastCommandToHorde(HordeID, RetreatCommand);
}

void UPortalAIHordeManager::BroadcastCommandToHorde(int32 HordeID, const FHordeCommand& Command)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    for (UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member) {
            Member->ProcessHordeCommand(Command);
        }
    }
}

void UPortalAIHordeManager::BroadcastCommandToAll(const FHordeCommand& Command)
{
    for (auto& Pair : ActiveHordes) {
        BroadcastCommandToHorde(Pair.Key, Command);
    }
}

int32 UPortalAIHordeManager::SpawnHorde(const UObject* WorldContextObject, const FVector& SpawnLocation,
    const FHordeSpawnConfig& Config)
{
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World || !Config.AIClass) {
        return -1;
    }

    TArray<UPortalAIHordeComponent*> SpawnedMembers;

    // Calculate spawn positions
    for (int32 i = 0; i < Config.SpawnCount; i++) {
        // Random position within spawn radius
        float RandomAngle = FMath::RandRange(0.0f, 2.0f * PI);
        float RandomDistance = FMath::RandRange(0.0f, Config.SpawnRadius);

        FVector SpawnPos = SpawnLocation + FVector(FMath::Cos(RandomAngle) * RandomDistance, FMath::Sin(RandomAngle) * RandomDistance, 0.0f);

        // Find valid spawn location on navmesh
        UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        if (NavSystem) {
            FNavLocation NavLocation;
            if (NavSystem->GetRandomReachablePointInRadius(SpawnPos, 100.0f, NavLocation)) {
                SpawnPos = NavLocation.Location;
            }
        }

        // Spawn AI
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        APawn* SpawnedPawn = World->SpawnActor<APawn>(Config.AIClass, SpawnPos, FRotator::ZeroRotator, SpawnParams);
        if (SpawnedPawn) {
            // Spawn and possess with controller
            APortalACFAIController* AIController = World->SpawnActor<APortalACFAIController>();
            if (AIController) {
                AIController->Possess(SpawnedPawn);
                AIController->SetDifficulty(Config.InitialDifficulty);

                // Get horde component
                if (UPortalAIHordeComponent* HordeComp = AIController->GetHordeComponent()) {
                    SpawnedMembers.Add(HordeComp);
                }
            }
        }
    }

    // Create horde from spawned members
    if (SpawnedMembers.Num() > 0) {
        int32 HordeID = CreateHorde(SpawnedMembers);

        // Set initial formation
        if (Config.InitialFormation != EHordeFormation::None) {
            SetHordeFormation(HordeID, Config.InitialFormation);
        }

        // Auto-engage if configured
        if (Config.bAutoEngageNearestTarget) {
            // Find nearest enemy
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
            if (PlayerPawn) {
                EngageTarget(HordeID, PlayerPawn);
            }
        }

        return HordeID;
    }

    return -1;
}

void UPortalAIHordeManager::DespawnHorde(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    // Destroy all pawns in horde
    for (UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member && Member->GetOwner()) {
            if (APortalACFAIController* Controller = Cast<APortalACFAIController>(Member->GetOwner())) {
                if (APawn* Pawn = Controller->GetPawn()) {
                    Pawn->Destroy();
                }
                Controller->Destroy();
            }
        }
    }

    // Destroy the horde
    DestroyHorde(HordeID);
}

FHordeGroup UPortalAIHordeManager::GetHordeInfo(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    return Horde ? *Horde : FHordeGroup();
}

TArray<int32> UPortalAIHordeManager::GetAllHordeIDs() const
{
    TArray<int32> HordeIDs;
    ActiveHordes.GetKeys(HordeIDs);
    return HordeIDs;
}

int32 UPortalAIHordeManager::GetHordeSize(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    return Horde ? Horde->Members.Num() : 0;
}

TArray<UPortalAIHordeComponent*> UPortalAIHordeManager::GetHordeMembers(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    return Horde ? Horde->Members : TArray<UPortalAIHordeComponent*>();
}

int32 UPortalAIHordeManager::FindNearestHorde(const FVector& Location) const
{
    int32 NearestHordeID = -1;
    float NearestDistance = MAX_FLT;

    for (const auto& Pair : ActiveHordes) {
        FVector HordeCenter = GetHordeCenter(Pair.Key);
        float Distance = FVector::Dist(Location, HordeCenter);

        if (Distance < NearestDistance) {
            NearestDistance = Distance;
            NearestHordeID = Pair.Key;
        }
    }

    return NearestHordeID;
}

bool UPortalAIHordeManager::IsValidHorde(int32 HordeID) const
{
    return ActiveHordes.Contains(HordeID);
}

float UPortalAIHordeManager::GetHordeThreatLevel(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return 0.0f;
    }

    float ThreatLevel = 0.0f;

    // Base threat from member count
    ThreatLevel += Horde->Members.Num() * 1.0f;

    // Bonus for having a leader
    if (Horde->Leader) {
        ThreatLevel += 2.0f;
    }

    // Bonus for being in combat
    if (Horde->CurrentState == EHordeState::Engaging) {
        ThreatLevel *= 1.5f;
    }

    // Bonus for advanced formations
    if (Horde->CurrentFormation == EHordeFormation::Flanking || Horde->CurrentFormation == EHordeFormation::Wedge) {
        ThreatLevel *= 1.2f;
    }

    return ThreatLevel;
}

float UPortalAIHordeManager::GetCombinedThreatLevel() const
{
    float TotalThreat = 0.0f;

    for (const auto& Pair : ActiveHordes) {
        TotalThreat += GetHordeThreatLevel(Pair.Key);
    }

    return TotalThreat;
}

FVector UPortalAIHordeManager::GetHordeCenter(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || Horde->Members.Num() == 0) {
        return FVector::ZeroVector;
    }

    FVector Center = FVector::ZeroVector;
    int32 ValidMembers = 0;

    for (const UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member && Member->GetOwner()) {
            if (APortalACFAIController* Controller = Cast<APortalACFAIController>(Member->GetOwner())) {
                if (APawn* Pawn = Controller->GetPawn()) {
                    Center += Pawn->GetActorLocation();
                    ValidMembers++;
                }
            }
        }
    }

    if (ValidMembers > 0) {
        Center /= ValidMembers;
    }

    return Center;
}

float UPortalAIHordeManager::GetHordeSpread(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || Horde->Members.Num() < 2) {
        return 0.0f;
    }

    FVector Center = GetHordeCenter(HordeID);
    float MaxDistance = 0.0f;

    for (const UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member && Member->GetOwner()) {
            if (APortalACFAIController* Controller = Cast<APortalACFAIController>(Member->GetOwner())) {
                if (APawn* Pawn = Controller->GetPawn()) {
                    float Distance = FVector::Dist(Pawn->GetActorLocation(), Center);
                    MaxDistance = FMath::Max(MaxDistance, Distance);
                }
            }
        }
    }

    return MaxDistance;
}

void UPortalAIHordeManager::UpdateHordes()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();

    // Clean up invalid members
    CleanupInvalidMembers();

    // Update horde states
    UpdateHordeStates();

    // Process merging if enabled
    if (bAutoMergeHordes) {
        ProcessHordeMerging();
    }

    // Process splitting if enabled
    if (bAutoSplitHordes) {
        ProcessHordeSplitting();
    }

    // Update horde centers
    for (auto& Pair : ActiveHordes) {
        RecalculateHordeCenter(Pair.Key);
        Pair.Value.LastUpdateTime = CurrentTime;
    }
}

void UPortalAIHordeManager::ProcessHordeMerging()
{
    TArray<int32> HordeIDs;
    ActiveHordes.GetKeys(HordeIDs);

    // Check all pairs of hordes for merging
    for (int32 i = 0; i < HordeIDs.Num(); i++) {
        for (int32 j = i + 1; j < HordeIDs.Num(); j++) {
            if (ShouldMergeHordes(HordeIDs[i], HordeIDs[j])) {
                MergeHordes(HordeIDs[i], HordeIDs[j]);
                return; // Only merge one pair per update
            }
        }
    }
}

void UPortalAIHordeManager::ProcessHordeSplitting()
{
    TArray<int32> HordeIDs;
    ActiveHordes.GetKeys(HordeIDs);

    for (int32 HordeID : HordeIDs) {
        if (ShouldSplitHorde(HordeID)) {
            SplitHorde(HordeID, 2);
            return; // Only split one horde per update
        }
    }
}

void UPortalAIHordeManager::UpdateHordeStates()
{
    for (auto& Pair : ActiveHordes) {
        FHordeGroup& Horde = Pair.Value;

        // Update state based on conditions
        switch (Horde.CurrentState) {
        case EHordeState::Engaging:
            // Check if target is still valid
            if (!IsValid(Horde.PrimaryTarget)) {
                Horde.CurrentState = EHordeState::Searching;
                Horde.PrimaryTarget = nullptr;
            }
            break;

        case EHordeState::Searching:
            // Look for new targets
            if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
                float Distance = FVector::Dist(GetHordeCenter(Pair.Key), PlayerPawn->GetActorLocation());
                if (Distance < 2000.0f) // Detection range
                {
                    EngageTarget(Pair.Key, PlayerPawn);
                }
            }
            break;

        case EHordeState::Retreating:
            // Check if we've retreated far enough
            if (Horde.PrimaryTarget) {
                float Distance = FVector::Dist(GetHordeCenter(Pair.Key), Horde.PrimaryTarget->GetActorLocation());
                if (Distance > 3000.0f) {
                    Horde.CurrentState = EHordeState::Regrouping;
                }
            }
            break;

        case EHordeState::Regrouping:
            // Check if all members are close together
            if (GetHordeSpread(Pair.Key) < FormationSpacing * 3.0f) {
                Horde.CurrentState = EHordeState::Idle;
            }
            break;
        }
    }
}

void UPortalAIHordeManager::CleanupInvalidMembers()
{
    for (auto& Pair : ActiveHordes) {
        FHordeGroup& Horde = Pair.Value;

        // Remove invalid members
        Horde.Members.RemoveAll([](const UPortalAIHordeComponent* Member) {
            return !Member || !IsValid(Member) || !Member->GetOwner();
        });

        // Update leader if needed
        if (!IsValid(Horde.Leader)) {
            Horde.Leader = nullptr;
            AutoAssignLeader(Pair.Key);
        }
    }

    // Remove empty hordes
    TArray<int32> EmptyHordes;
    for (const auto& Pair : ActiveHordes) {
        if (Pair.Value.Members.Num() == 0) {
            EmptyHordes.Add(Pair.Key);
        }
    }

    for (int32 HordeID : EmptyHordes) {
        DestroyHorde(HordeID);
    }
}

int32 UPortalAIHordeManager::GenerateHordeID()
{
    return NextHordeID++;
}

void UPortalAIHordeManager::RecalculateHordeCenter(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    Horde->HordeCenter = GetHordeCenter(HordeID);
}

void UPortalAIHordeManager::AssignFormationPositions(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde || Horde->CurrentFormation == EHordeFormation::None) {
        return;
    }

    int32 MemberIndex = 0;
    for (UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member) {
            FVector FormationPos = Member->CalculateFormationPosition(
                MemberIndex,
                Horde->Members.Num(),
                Horde->CurrentFormation,
                Horde->HordeCenter);

            Member->AssignedFormationPosition = FormationPos;
            MemberIndex++;
        }
    }
}

bool UPortalAIHordeManager::ShouldMergeHordes(int32 HordeID1, int32 HordeID2) const
{
    const FHordeGroup* Horde1 = ActiveHordes.Find(HordeID1);
    const FHordeGroup* Horde2 = ActiveHordes.Find(HordeID2);

    if (!Horde1 || !Horde2) {
        return false;
    }

    // Don't merge if combined size would be too large
    if (Horde1->Members.Num() + Horde2->Members.Num() > MaxHordeSize) {
        return false;
    }

    // Check distance between hordes
    float Distance = FVector::Dist(GetHordeCenter(HordeID1), GetHordeCenter(HordeID2));
    if (Distance > HordeMergeDistance) {
        return false;
    }

    // Check if they have the same target
    if (Horde1->PrimaryTarget == Horde2->PrimaryTarget && Horde1->PrimaryTarget != nullptr) {
        return true;
    }

    // Check if both are idle or patrolling
    if ((Horde1->CurrentState == EHordeState::Idle || Horde1->CurrentState == EHordeState::Patrolling) && (Horde2->CurrentState == EHordeState::Idle || Horde2->CurrentState == EHordeState::Patrolling)) {
        return true;
    }

    return false;
}

bool UPortalAIHordeManager::ShouldSplitHorde(int32 HordeID) const
{
    const FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return false;
    }

    // Don't split small hordes
    if (Horde->Members.Num() < 4) {
        return false;
    }

    // Check spread
    float Spread = GetHordeSpread(HordeID);
    if (Spread > HordeSplitDistance) {
        return true;
    }

    return false;
}

void UPortalAIHordeManager::ValidateHordeIntegrity(int32 HordeID)
{
    FHordeGroup* Horde = ActiveHordes.Find(HordeID);
    if (!Horde) {
        return;
    }

    // Ensure all members know they're in this horde
    for (UPortalAIHordeComponent* Member : Horde->Members) {
        if (Member && Member->GetHordeID() != HordeID) {
            Member->JoinHorde(HordeID);
        }
    }

    // Ensure we have a leader
    if (!Horde->Leader && Horde->Members.Num() > 0) {
        AutoAssignLeader(HordeID);
    }
}