#include "ACFStealthDetectionComponent.h"
#include "ACFAIController.h"
#include "Components/ACFThreatManagerComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Game/ACFFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ACFEntityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "PortalStealthConfigDataAsset.h"

UACFStealthDetectionComponent::UACFStealthDetectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    bEnableStealthDetection = true;
    bOverrideACFDetection = true;
    LastDetectionType = EStealthDetectionType::None;
    bInvestigatingSound = false;

    VegetationTags.Add(FName("Vegetation"));
    VegetationTags.Add(FName("Tree"));
    VegetationTags.Add(FName("Bush"));
    VegetationTags.Add(FName("Foliage"));

    GrassTags.Add(FName("Grass"));
    GrassTags.Add(FName("LongGrass"));
    GrassTags.Add(FName("Weeds"));

    StealthSettings = FHybridStealthSettings();
}

void UACFStealthDetectionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn) {
        ACFController = Cast<AACFAIController>(OwnerPawn->GetController());
    }

    if (StealthConfigAsset) {
        ApplyStealthConfiguration();
    }

    InitializeWithACFController();
}

void UACFStealthDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnableStealthDetection || !ACFController) {
        return;
    }

    if (bOverrideACFDetection) {
        CheckForStealthThreats();
    }
}

void UACFStealthDetectionComponent::InitializeWithACFController()
{
    if (!ACFController)
        return;

    // Hook into ACF's perception system
    if (UAIPerceptionComponent* PerceptionComp = ACFController->GetPerceptionComponent()) {
        if (!PerceptionComp->OnTargetPerceptionUpdated.IsAlreadyBound(this, &UACFStealthDetectionComponent::OnACFPerceptionUpdated)) {
            PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UACFStealthDetectionComponent::OnACFPerceptionUpdated);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Stealth Detection Component initialized for %s"), *GetOwner()->GetName());
}

void UACFStealthDetectionComponent::OnACFPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!bEnableStealthDetection || !bOverrideACFDetection)
        return;

    APawn* PlayerPawn = Cast<APawn>(Actor);
    if (!PlayerPawn || !PlayerPawn->IsPlayerControlled())
        return;

    float DetectionRange;
    EStealthDetectionType DetectionType;

    // Use our stealth detection instead of standard ACF detection
    if (PerformStealthDetection(PlayerPawn, DetectionRange, DetectionType)) {
        NotifyACFControllerOfDetection(PlayerPawn, DetectionType);
    }
}

bool UACFStealthDetectionComponent::PerformStealthDetection(APawn* TargetPlayer, float& OutDetectionRange, EStealthDetectionType& OutDetectionType)
{
    if (!TargetPlayer || !OwnerPawn) {
        OutDetectionRange = 0.0f;
        OutDetectionType = EStealthDetectionType::None;
        return false;
    }

    float DistanceToPlayer = FVector::Dist(OwnerPawn->GetActorLocation(), TargetPlayer->GetActorLocation());

    // PRIORITY 1: Light Detection (Instant Aggro)
    if (StealthSettings.bInstantAggroInLight && IsPlayerIlluminated(TargetPlayer)) {
        if (DistanceToPlayer <= StealthSettings.LightAggroRange && HasLineOfSightToPlayer(TargetPlayer)) {
            OutDetectionRange = StealthSettings.LightAggroRange;
            OutDetectionType = EStealthDetectionType::LightAggro;
            return true;
        }
    }

    // PRIORITY 2: Darkness Detection
    if (ShouldUseDarknessDetection(TargetPlayer)) {
        // Audio Detection
        if (CanHearPlayer(TargetPlayer)) {
            OutDetectionRange = CalculatePlayerNoiseLevel(TargetPlayer);
            OutDetectionType = EStealthDetectionType::Audio;
            return true;
        }

        // Close Visual Detection in Darkness
        if (DistanceToPlayer <= StealthSettings.DarknessVisualRange && HasLineOfSightToPlayer(TargetPlayer)) {
            OutDetectionRange = StealthSettings.DarknessVisualRange;
            OutDetectionType = EStealthDetectionType::Visual;
            return true;
        }
    }

    OutDetectionRange = 0.0f;
    OutDetectionType = EStealthDetectionType::None;
    return false;
}

bool UACFStealthDetectionComponent::IsPlayerIlluminated(APawn* Player)
{
    if (!Player)
        return false;

    FVector PlayerLocation = Player->GetActorLocation();
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors) {
        TArray<ULightComponent*> LightComponents;
        Actor->GetComponents<ULightComponent>(LightComponents);

        for (ULightComponent* LightComp : LightComponents) {
            if (LightComp && LightComp->IsVisible()) {
                float DistanceToLight = FVector::Dist(PlayerLocation, Actor->GetActorLocation());
                float LightRadius = StealthSettings.LightDetectionRadius;

                if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(LightComp)) {
                    LightRadius = PointLight->AttenuationRadius;
                } else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(LightComp)) {
                    LightRadius = SpotLight->AttenuationRadius;
                }

                if (LightRadius <= 0.0f) {
                    LightRadius = StealthSettings.LightDetectionRadius;
                }

                if (DistanceToLight <= FMath::Min(LightRadius, StealthSettings.LightDetectionRadius)) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool UACFStealthDetectionComponent::CanHearPlayer(APawn* Player)
{
    if (!Player)
        return false;

    float PlayerNoiseRange = CalculatePlayerNoiseLevel(Player);
    if (PlayerNoiseRange <= 0.0f)
        return false;

    float DistanceToPlayer = FVector::Dist(OwnerPawn->GetActorLocation(), Player->GetActorLocation());
    return DistanceToPlayer <= PlayerNoiseRange;
}

float UACFStealthDetectionComponent::CalculatePlayerNoiseLevel(APawn* Player)
{
    if (!Player)
        return 0.0f;

    FVector PlayerVelocity = Player->GetVelocity();
    float PlayerSpeed = PlayerVelocity.Size();

    float BaseNoiseRange = 0.0f;

    if (PlayerSpeed < 50.0f) {
        BaseNoiseRange = 0.0f;
    } else if (PlayerSpeed < 200.0f) {
        BaseNoiseRange = StealthSettings.CrouchingNoiseRange;
    } else if (PlayerSpeed < 400.0f) {
        BaseNoiseRange = StealthSettings.WalkingNoiseRange;
    } else {
        BaseNoiseRange = StealthSettings.RunningNoiseRange;
    }

    if (IsPlayerInVegetation(Player)) {
        if (PlayerSpeed > 100.0f) {
            BaseNoiseRange *= StealthSettings.VegetationNoiseMultiplier;
        }
    }

    return BaseNoiseRange;
}

bool UACFStealthDetectionComponent::IsPlayerInVegetation(APawn* Player)
{
    if (!Player)
        return false;

    FVector PlayerLocation = Player->GetActorLocation();

    for (const FName& Tag : VegetationTags) {
        TArray<AActor*> VegetationActors;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, VegetationActors);

        for (AActor* VegActor : VegetationActors) {
            float Distance = FVector::Dist(PlayerLocation, VegActor->GetActorLocation());
            if (Distance < 200.0f) {
                return true;
            }
        }
    }

    for (const FName& Tag : GrassTags) {
        TArray<AActor*> GrassActors;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, GrassActors);

        for (AActor* GrassActor : GrassActors) {
            float Distance = FVector::Dist(PlayerLocation, GrassActor->GetActorLocation());
            if (Distance < 150.0f) {
                return true;
            }
        }
    }

    return false;
}

bool UACFStealthDetectionComponent::ShouldUseDarknessDetection(APawn* Player)
{
    return !IsPlayerIlluminated(Player);
}

void UACFStealthDetectionComponent::TriggerLightAggro(APawn* Player)
{
    if (!Player || !ACFController)
        return;

    LastDetectionType = EStealthDetectionType::LightAggro;

    if (UACFThreatManagerComponent* ThreatManager = ACFController->GetThreatManager()) {
        float LightThreat = 1000.0f;
        ThreatManager->AddThreat(Player, LightThreat);
    }

    ACFController->SetTarget(Player);
    ACFController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));

    if (StealthSettings.bAlertOtherGuardsOnLightDetection) {
        AlertNearbyGuards(Player);
    }

    UE_LOG(LogTemp, Warning, TEXT("%s: LIGHT AGGRO - Player spotted in light!"), *GetOwner()->GetName());
}

void UACFStealthDetectionComponent::AlertNearbyGuards(APawn* Player)
{
    if (!Player)
        return;

    FVector PlayerLocation = Player->GetActorLocation();
    TArray<AActor*> NearbyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), NearbyActors);

    for (AActor* Actor : NearbyActors) {
        if (APawn* OtherPawn = Cast<APawn>(Actor)) {
            if (OtherPawn == OwnerPawn || OtherPawn == Player)
                continue;

            float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), OtherPawn->GetActorLocation());
            if (Distance <= StealthSettings.AggroAlertRadius) {
                if (UACFStealthDetectionComponent* OtherStealth = OtherPawn->FindComponentByClass<UACFStealthDetectionComponent>()) {
                    OtherStealth->StartSoundInvestigation(PlayerLocation);
                }
            }
        }
    }
}

void UACFStealthDetectionComponent::StartSoundInvestigation(FVector SoundLocation)
{
    if (!ACFController)
        return;

    bInvestigatingSound = true;
    LastDetectionType = EStealthDetectionType::Audio;

    ACFController->SetTargetLocationBK(SoundLocation);

    GetWorld()->GetTimerManager().SetTimer(SoundInvestigationTimer, this, &UACFStealthDetectionComponent::OnSoundInvestigationComplete, StealthSettings.SoundInvestigationDuration, false);

    UE_LOG(LogTemp, Log, TEXT("%s investigating sound at %s"), *GetOwner()->GetName(), *SoundLocation.ToString());
}

void UACFStealthDetectionComponent::ApplyStealthConfiguration()
{
    if (!StealthConfigAsset) {
        return;
    }

    StealthSettings = StealthConfigAsset->GetStealthSettings();
    VegetationTags = StealthConfigAsset->VegetationTags;
    GrassTags = StealthConfigAsset->GrassTags;

    UE_LOG(LogTemp, Log, TEXT("Applied stealth configuration to %s"), *GetOwner()->GetName());
}

void UACFStealthDetectionComponent::SetStealthConfigAsset(UPortalStealthConfigDataAsset* NewConfigAsset)
{
    StealthConfigAsset = NewConfigAsset;
    if (StealthConfigAsset) {
        ApplyStealthConfiguration();
    }
}

void UACFStealthDetectionComponent::OverrideACFDetection(bool bEnable)
{
    bOverrideACFDetection = bEnable;
}

void UACFStealthDetectionComponent::CheckForStealthThreats()
{
    if (!ACFController || !OwnerPawn)
        return;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundActors);

    APawn* DetectedPlayerTarget = nullptr;
    float NearestDistance = FLT_MAX;
    EStealthDetectionType DetectionMethod = EStealthDetectionType::None;

    PlayersInHearingRange.Empty();

    for (AActor* Actor : FoundActors) {
        APawn* PlayerPawn = Cast<APawn>(Actor);
        if (!PlayerPawn || PlayerPawn == OwnerPawn || !PlayerPawn->IsPlayerControlled()) {
            continue;
        }

        float DetectionRange;
        EStealthDetectionType DetectionType;

        if (PerformStealthDetection(PlayerPawn, DetectionRange, DetectionType)) {
            IACFEntityInterface* Entity = Cast<IACFEntityInterface>(PlayerPawn);
            if (Entity && IACFEntityInterface::Execute_IsEntityAlive(PlayerPawn)) {
                ETeam PlayerTeam = IACFEntityInterface::Execute_GetEntityCombatTeam(PlayerPawn);

                if (UACFFunctionLibrary::AreEnemyTeams(GetWorld(), ACFController->GetCombatTeam(), PlayerTeam)) {
                    float DistanceToPlayer = FVector::Dist(OwnerPawn->GetActorLocation(), PlayerPawn->GetActorLocation());

                    if (DistanceToPlayer < NearestDistance) {
                        DetectedPlayerTarget = PlayerPawn;
                        NearestDistance = DistanceToPlayer;
                        DetectionMethod = DetectionType;
                    }

                    if (DetectionType == EStealthDetectionType::Audio) {
                        PlayersInHearingRange.Add(PlayerPawn);
                    }
                }
            }
        }
    }

    if (DetectedPlayerTarget) {
        NotifyACFControllerOfDetection(DetectedPlayerTarget, DetectionMethod);
    }
}

bool UACFStealthDetectionComponent::HasLineOfSightToPlayer(APawn* Player)
{
    if (!Player || !OwnerPawn)
        return false;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerPawn);
    QueryParams.AddIgnoredActor(Player);

    return !GetWorld()->LineTraceSingleByChannel(
        HitResult,
        OwnerPawn->GetActorLocation(),
        Player->GetActorLocation(),
        ECC_Visibility,
        QueryParams);
}

void UACFStealthDetectionComponent::NotifyACFControllerOfDetection(APawn* Player, EStealthDetectionType DetectionType)
{
    if (!ACFController || !Player)
        return;

    LastDetectionType = DetectionType;

    if (UACFThreatManagerComponent* ThreatManager = ACFController->GetThreatManager()) {
        float ThreatLevel = 500.0f;
        if (DetectionType == EStealthDetectionType::LightAggro) {
            ThreatLevel *= 2.0f;
        }
        ThreatManager->AddThreat(Player, ThreatLevel);
    }

    if (DetectionType == EStealthDetectionType::LightAggro) {
        TriggerLightAggro(Player);
    } else if (DetectionType == EStealthDetectionType::Audio) {
        StartSoundInvestigation(Player->GetActorLocation());
    } else {
        ACFController->SetTarget(Player);
        ACFController->SetCurrentAIState(UACFFunctionLibrary::GetAIStateTag(EAIState::EBattle));
    }

    UE_LOG(LogTemp, Log, TEXT("%s detected player via %s"),
        *GetOwner()->GetName(),
        DetectionType == EStealthDetectionType::LightAggro ? TEXT("LIGHT") : DetectionType == EStealthDetectionType::Audio ? TEXT("SOUND")
                                                                                                                           : TEXT("SIGHT"));
}

void UACFStealthDetectionComponent::OnSoundInvestigationComplete()
{
    bInvestigatingSound = false;
    if (ACFController) {
        ACFController->ResetToDefaultState();
    }
}