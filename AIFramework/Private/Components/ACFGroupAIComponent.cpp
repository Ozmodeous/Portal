// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFGroupAIComponent.h"
#include "ACFAIController.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFThreatManagerComponent.h"
#include "Game/ACFFunctionLibrary.h"
#include "Game/ACFPlayerController.h"
#include "Game/ACFTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include <Engine/World.h>
#include <GameFramework/Pawn.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetSystemLibrary.h>
#include <NavigationSystem.h>

// Sets default values for this component's properties
UACFGroupAIComponent::UACFGroupAIComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bInBattle = false;
    SetIsReplicatedByDefault(true);
    DefaultSpawnOffset = FVector2D(150.f, 150.f);
}

void UACFGroupAIComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UACFGroupAIComponent, groupLead);
    DOREPLIFETIME(UACFGroupAIComponent, bInBattle);
    DOREPLIFETIME(UACFGroupAIComponent, AICharactersInfo);
}
// Called when the game starts
void UACFGroupAIComponent::BeginPlay()
{
    Super::BeginPlay();
    SetReferences();
}

void UACFGroupAIComponent::SetReferences()
{
    groupLead = Cast<AActor>(GetOwner());
}

void UACFGroupAIComponent::OnComponentLoaded_Implementation()
{
    for (int32 index = 0; index < AICharactersInfo.Num(); index++) {
        FAIAgentsInfo& agent = AICharactersInfo[index];
        TArray<AActor*> foundActors;
        UGameplayStatics::GetAllActorsOfClassWithTag(this, agent.characterClass, agent.Guid, foundActors);
        if (foundActors.Num() == 0) {
            UE_LOG(LogTemp, Error, TEXT("Impossible to find actor"));
            continue;
        }
        agent.AICharacter = Cast<AACFCharacter>(foundActors[0]);
        InitAgent(agent, index);
    }
}

void UACFGroupAIComponent::SendCommandToCompanions_Implementation(FGameplayTag command)
{
    Internal_SendCommandToAgents(command);
}

void UACFGroupAIComponent::SpawnGroup_Implementation()
{
    if (bAlreadySpawned && !bCanSpawnMultitpleTimes) {
        return;
    }

    if (AICharactersInfo.Num() > 0)
    {
        // Already spawned!
        return;
    }

    Internal_SpawnGroup();
    bAlreadySpawned = true;
}

void UACFGroupAIComponent::DespawnGroup_Implementation(const bool bUpdateAIToSpawn /*= true*/, FGameplayTag actionToTriggerOnDyingAgent, float lifespawn /*= 1.f*/)
{
    if (bAlreadySpawned) {
        if (bUpdateAIToSpawn) {
          //  TArray<FAISpawnInfo> aicopy = AIToSpawn;
            AIToSpawn.Empty();
            for (FAIAgentsInfo agent : AICharactersInfo) {
                if (agent.AICharacter && agent.AICharacter->IsAlive()) {
                    const TSubclassOf<AACFCharacter> charClass = agent.AICharacter->GetClass();
                    AddAIToSpawn(FAISpawnInfo(charClass));

                }
            }
        }
        for (FAIAgentsInfo& agent : AICharactersInfo) {
            if (agent.AICharacter && agent.AICharacter->IsAlive()) {
                agent.AICharacter->DestroyCharacter(lifespawn);
                agent.AICharacter->TriggerAction(actionToTriggerOnDyingAgent, EActionPriority::EHigh);
            }
        }
        AICharactersInfo.Empty();
        bAlreadySpawned = false;
        OnAgentsDespawned.Broadcast();
    }
}

void UACFGroupAIComponent::InitAgents()
{
    for (int32 index = 0; index < AICharactersInfo.Num(); index++) {
        if (AICharactersInfo.IsValidIndex(index)) {
            InitAgent(AICharactersInfo[index], index);
        }
    }
}

void UACFGroupAIComponent::InitAgent(FAIAgentsInfo& agent, int32 childIndex)
{
    if (!agent.AICharacter) {
        ensure(false);
        return;
    }

    if (!agent.AICharacter->GetController()) {
        agent.AICharacter->SpawnDefaultController();
    }

    agent.characterClass = (agent.AICharacter->GetClass());

    if (!AIToSpawn.IsValidIndex(childIndex)) {
        UE_LOG(LogTemp, Error, TEXT("invalid AI Index! - UACFGroupAIComponent::InitAgent"), *this->GetName());

        return;
    }
    ensure(agent.characterClass == AIToSpawn[childIndex].AIClassBP);
    if (agent.GetController()) {
        if (!groupLead) {
            SetReferences();
        }
        agent.GetController()->SetLeadActorBK(groupLead);
        agent.GetController()->SetDefaultState(DefaultAiState);
        agent.GetController()->SetCurrentAIState(DefaultAiState);
        if (AIToSpawn[childIndex].PatrolPath) {
                agent.GetController()->SetPatrolPath(AIToSpawn[childIndex].PatrolPath, true);

        }
        if (bOverrideAgentTeam) {
            agent.AICharacter->AssignTeam(CombatTeam);
        }

        check(agent.characterClass);
        const FName newGuid = agent.AICharacter->GetFName();
        agent.Guid = newGuid;
        if (!agent.AICharacter->Tags.Contains(newGuid)) {         
            agent.AICharacter->Tags.Add(newGuid);        
        }
        agent.GetController()->SetGroupOwner(this, childIndex, bOverrideAgentPerception, bOverrideAgentTeam);
        if (!agent.AICharacter->OnDeath.IsAlreadyBound(this, &UACFGroupAIComponent::HandleAgentDeath)) {
            agent.AICharacter->OnDeath.AddDynamic(this, &UACFGroupAIComponent::HandleAgentDeath);
        }
    }
}

bool UACFGroupAIComponent::AddAIToSpawnFromClass(const TSubclassOf<AACFCharacter>& charClass)
{
    if (GetMaxSimultaneousAgents() <= GetTotalAIToSpawnCount()) {
        UE_LOG(LogTemp, Warning, TEXT("Your Group Already Reach The Maximum! - UACFGroupAIComponent::AddAIToSpawn"), *this->GetName());
        return false;
    }

    AIToSpawn.Add(FAISpawnInfo(charClass));

    OnAgentsChanged.Broadcast();
    return true;
}

bool UACFGroupAIComponent::AddAIToSpawn(const FAISpawnInfo& spawnInfo)
{
    if (GetMaxSimultaneousAgents() < GetTotalAIToSpawnCount()) {
        UE_LOG(LogTemp, Warning, TEXT("Your Group Already Reach The Maximum! - UACFGroupAIComponent::AddAIToSpawn"), *this->GetName());
        return false;
    }

    AIToSpawn.Add(spawnInfo);

    OnAgentsChanged.Broadcast();
    return true;
}

bool UACFGroupAIComponent::RemoveAIToSpawn(const TSubclassOf<AACFCharacter>& charClass)
{
    if (AIToSpawn.Contains(charClass)) {

        AIToSpawn.Remove(charClass);

        OnAgentsChanged.Broadcast();
        return true;
    }
    return false;
}

void UACFGroupAIComponent::ReplaceAIToSpawn(const TArray<FAISpawnInfo>& newAIs)
{
    AIToSpawn.Empty();
    AIToSpawn = newAIs;
}

bool UACFGroupAIComponent::GetAgentByIndex(int32 index, FAIAgentsInfo& outAgent) const
{
    if (AICharactersInfo.IsValidIndex(index)) {
        outAgent = AICharactersInfo[index];
        return true;
    }
    return false;
}

void UACFGroupAIComponent::Internal_SendCommandToAgents(FGameplayTag command)
{
    for (FAIAgentsInfo& achar : AICharactersInfo) {
        if (achar.GetController()) {
            achar.GetController()->TriggerCommand(command);
        } else {

            ensure(false);
        }
    }
}

void UACFGroupAIComponent::SetEnemyGroup(UACFGroupAIComponent* inEnemyGroup)
{
    if (inEnemyGroup && UACFFunctionLibrary::AreEnemyTeams(GetWorld(), GetCombatTeam(), inEnemyGroup->GetCombatTeam())) {
        enemyGroup = inEnemyGroup;
    }
}

void UACFGroupAIComponent::HandleAgentDeath(class AACFCharacter* agent)
{
    OnChildDeath(agent);
}

FVector UACFGroupAIComponent::GetGroupCentroid() const
{
    TArray<AActor*> actors;
    for (const auto& agent : AICharactersInfo) {
        if (agent.AICharacter) {
            actors.Add(agent.AICharacter);
        }
    }
    return UGameplayStatics::GetActorArrayAverageLocation(actors);
}

class AACFCharacter* UACFGroupAIComponent::RequestNewTarget(const AACFAIController* requestSender)
{
    // First Try to help lead
    const AACFCharacter* lead = Cast<AACFCharacter>(requestSender->GetLeadActorBK());
    if (lead) {
        const AACFCharacter* newTarget = Cast<AACFCharacter>(lead->GetTarget());
        if (newTarget && newTarget->IsMyEnemy(requestSender->GetBaseAICharacter())) {
            return Cast<AACFCharacter>(lead->GetTarget());
        }
    }

    // Then Try to help other in  the group
    if (AICharactersInfo.IsValidIndex(0) && IsValid(AICharactersInfo[0].AICharacter) && IsValid(AICharactersInfo[0].GetController())) {
        for (FAIAgentsInfo achar : AICharactersInfo) {
            if (achar.GetController() && achar.GetController() != requestSender) {
                AACFCharacter* newTarget = Cast<AACFCharacter>(achar.GetController()->GetTargetActorBK());
                if (newTarget && newTarget->IsAlive() && achar.GetController()->IsInBattle()) {
                    return newTarget;
                }
            }
        }
    }

    // Then Try to help other in  the group
    if (enemyGroup) {
        return enemyGroup->GetAgentNearestTo(requestSender->GetPawn()->GetActorLocation());
    }

    return nullptr;
}

void UACFGroupAIComponent::Internal_SpawnGroup()
{
    if (AIToSpawn.Num() > 0) {
        const UWorld* world = GetWorld();
        if (world) {

            int32 currentIndex = 0;
            for (auto& aiSpawn : AIToSpawn) {
                const int32 childGroupIndex = AddAgentToGroup(aiSpawn);
            }
        }
        bAlreadySpawned = true;
        OnAgentsSpawned.Broadcast();
    } else {
        UE_LOG(LogTemp, Error, TEXT("%s NO AI to Spawn - AAIFGROUPPAWN::SpawnAGroup"), *this->GetName());
    }
}

uint8 UACFGroupAIComponent::AddAgentToGroup(const FAISpawnInfo& spawnInfo)
{
    UWorld* const world = GetWorld();

    ensure(GetOwner()->HasAuthority());

    if (!world)
        return -1;

    if (!groupLead) {
        SetReferences();
        if (!groupLead) {
            return -1;
        }
    }

    if (AICharactersInfo.Num() >= GetMaxSimultaneousAgents()) {
        return -1;
    }

    FAIAgentsInfo newCharacterInfo;

    const int32 localGroupIndex = AICharactersInfo.Num();
    const FVector myLocation = groupLead->GetActorLocation();
    FVector additivePos = FVector::ZeroVector;
    FActorSpawnParameters spawnParam;
    spawnParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    FTransform spawnTransform;
    if (spawnInfo.SpawnTransform.GetLocation() != FVector::ZeroVector) {
        additivePos = spawnInfo.SpawnTransform.GetLocation();
    } else {
        additivePos.X = UKismetMathLibrary::RandomFloatInRange(-DefaultSpawnOffset.X, DefaultSpawnOffset.X);
        additivePos.Y = UKismetMathLibrary::RandomFloatInRange(-DefaultSpawnOffset.Y, DefaultSpawnOffset.Y);
    }
    const FVector spawnLocation = myLocation + additivePos;
    FVector outPoint;

    if (UNavigationSystemV1::K2_ProjectPointToNavigation(this, spawnLocation, outPoint, nullptr, nullptr, FVector(500.f))) {
        spawnTransform.SetLocation(outPoint);
       
    } else {
        spawnTransform.SetLocation(spawnLocation);
    }
    spawnTransform.SetRotation(spawnInfo.SpawnTransform.GetRotation());
    newCharacterInfo.AICharacter = world->SpawnActorDeferred<AACFCharacter>(
        spawnInfo.AIClassBP, spawnTransform, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    if (newCharacterInfo.AICharacter) {

        UGameplayStatics::FinishSpawningActor(newCharacterInfo.AICharacter, spawnTransform);

        // End Spawn
        if (!newCharacterInfo.AICharacter->GetController()) {
            newCharacterInfo.AICharacter->SpawnDefaultController();
        }
        InitAgent(newCharacterInfo, localGroupIndex);

        AICharactersInfo.Add(newCharacterInfo);
        return localGroupIndex;
    }
    return -1;
}

int32 UACFGroupAIComponent::GetTotalAIToSpawnCount() const
{

    return AIToSpawn.Num();
}

bool UACFGroupAIComponent::AddExistingCharacterToGroup(AACFCharacter* character)
{
    const UWorld* world = GetWorld();

    if (!world) {
        return false;
    }

    if (!groupLead) {
        SetReferences();
    }

    if (AICharactersInfo.Contains(character)) {
        InitAgents();
        return true;
    }

    FAIAgentsInfo newCharacterInfo;
    newCharacterInfo.AICharacter = character;

    if (newCharacterInfo.AICharacter) {
        if (!newCharacterInfo.AICharacter->GetController()) {
            newCharacterInfo.AICharacter->SpawnDefaultController();
        }

        uint8 childIndex = AICharactersInfo.Num();
        newCharacterInfo.GetController() = Cast<AACFAIController>(newCharacterInfo.AICharacter->GetController());
        if (newCharacterInfo.GetController()) {
            InitAgent(newCharacterInfo, childIndex);
        } else {
            UE_LOG(LogTemp, Error, TEXT("%s NO AI to Spawn - AAIFGROUPPAWN::SpawnAGroup"), *this->GetName());
        }

        AICharactersInfo.Add(newCharacterInfo);
        return true;
    }
    return false;
}

void UACFGroupAIComponent::ReInitAgent(AACFCharacter* character)
{
    if (AICharactersInfo.Contains(character)) {
        FAIAgentsInfo* newCharacterInfo = AICharactersInfo.FindByKey(character);
        const int32 index = AICharactersInfo.IndexOfByKey(character);
        InitAgent(*newCharacterInfo, index);
    }
}

AACFCharacter* UACFGroupAIComponent::GetAgentNearestTo(const FVector& location) const
{
    AACFCharacter* bestAgent = nullptr;
    float minDistance = 999999.f;
    for (FAIAgentsInfo achar : AICharactersInfo) {
        if (achar.AICharacter && achar.AICharacter->IsAlive()) {
            const float distance = FVector::Distance(location, achar.AICharacter->GetActorLocation());
            if (distance <= minDistance) {
                minDistance = distance;
                bestAgent = achar.AICharacter;
            }
        }
    }
    return bestAgent;
}

bool UACFGroupAIComponent::RemoveAgentFromGroup(AACFCharacter* character)
{
    if (!character) {
        return false;
    }

    AACFAIController* contr = Cast<AACFAIController>(character->GetController());
    if (!contr) {
        return false;
    }

    const FAIAgentsInfo agentInfo(character);

    if (AICharactersInfo.Contains(agentInfo)) {
        AICharactersInfo.RemoveSingle(agentInfo);
        return true;
    }

    return false;
}

void UACFGroupAIComponent::SetInBattle(bool inBattle, AActor* newTarget)
{
    bInBattle = inBattle;
    if (bInBattle) {
        const APawn* aiTarget = Cast<APawn>(newTarget);
        if (aiTarget) {

            // Check if target is part of a group
            AController* targetCont = aiTarget->GetController();
            if (targetCont) {
                const bool bImplements = targetCont->GetClass()->ImplementsInterface(UACFGroupAgentInterface::StaticClass());
                if (bImplements && IACFGroupAgentInterface::Execute_IsPartOfGroup(targetCont)) {
                    UACFGroupAIComponent* groupComp = IACFGroupAgentInterface::Execute_GetOwningGroup(targetCont);
                    SetEnemyGroup(groupComp);
                } else {
                    enemyGroup = nullptr;
                }
            }
        }

        int32 index = 0;
        for (const FAIAgentsInfo& achar : AICharactersInfo) {
            if (!achar.GetController() || achar.GetController()->IsInBattle() || !achar.AICharacter->IsAlive()) {
                continue;
            }

            // Trying to assign to every agent in the group that is not in battle an enemy in the enemy group
            AActor* nextTarget = newTarget;
            FAIAgentsInfo adversary;
            if (enemyGroup && enemyGroup->GetGroupSize() > 0) {
                if (achar.GetController() && !achar.GetController()->HasTarget()) {
                    if (enemyGroup->GetGroupSize() > index) {
                        enemyGroup->GetAgentByIndex(index, adversary);
                        index++;
                    } else {
                        index = 0;
                        enemyGroup->GetAgentByIndex(index, adversary);
                        index++;
                    }
                    nextTarget = adversary.AICharacter;
                }
            }
            UACFThreatManagerComponent* threatComp = achar.GetController()->GetThreatManager();
            if (nextTarget) {
                const float newThreat = threatComp->GetDefaultThreatForActor(nextTarget);
                if (newThreat > 0.f) {
                    // if the enemy we found is valid, we add aggro for that enemy
                    threatComp->AddThreat(nextTarget, newThreat + 10.f);
                } else {
                    // otherwise we go in the new target
                    threatComp->AddThreat(newTarget, threatComp->GetDefaultThreatForActor(newTarget));
                }
            }
        }
    }
}

void UACFGroupAIComponent::OnChildDeath(const AACFCharacter* character)
{
    const int32 index = AICharactersInfo.IndexOfByKey(character);
    if (AICharactersInfo.IsValidIndex(index)) {
        AICharactersInfo.RemoveAt(index);
    }
    OnAgentDeath.Broadcast(character);
    if (AICharactersInfo.Num() == 0) {
        OnAllAgentDeath.Broadcast();
    }
}
