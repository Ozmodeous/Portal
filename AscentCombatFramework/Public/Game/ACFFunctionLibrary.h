// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFDamageType.h"
#include "ACFTypes.h"
#include "ARSTypes.h"
#include "CoreMinimal.h"
#include "Items/ACFItem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>

#include "ACFFunctionLibrary.generated.h"

class AACFCharacter;
struct FAttributesSet;
struct FStartingItem;
class UARSStatisticsComponent;
class AActor;

enum class EAIState : uint8;


/**
 *  UACFFunctionLibrary
 *  A utility Blueprint Function Library that provides helper functions for various gameplay mechanics in the
 *  Ascent Combat Framework (ACF). This library includes functions for damage calculation, directional hit detection,
 *  battle types, distance calculations, character interactions, and game state retrieval.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFFunctionLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    // Determines the hit direction based on the attacker (hitDealer) and the hit result.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static EACFDirection GetHitDirectionByHitResult(const AActor* hitDealer, const FHitResult& hitResult);

    // Gets the relative direction vector of a damage event.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static FVector GetActorsRelativeDirectionVector(const FACFDamageEvent& damageEvent);

    // Determines the relative direction of a hit based on the attacker's and receiver's positions.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static EACFDirection GetActorsRelativeDirection(const AActor* hitDealer, const AActor* receiver);

    // Determines movement direction based on input.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static EACFDirection GetDirectionFromInput(const AActor* actor, const FVector& direction);

    // Gets the opposite relative direction of a given damage event.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static FVector GetActorsOppositeRelativeDirection(const FACFDamageEvent& damageEvent);

    // Retrieves the default gameplay tag representing the death state.
    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static FGameplayTag GetDefaultDeathState();

    // Retrieves the default gameplay tag representing the hit state.
    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static FGameplayTag GetDefaultHitState();

    // Retrieves the gameplay tag associated with health.
    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static FGameplayTag GetHealthTag();

     UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static FGameplayTag GetAIStateTag(EAIState aiState);

    // Reduces a given damage amount by a specified percentage.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float ReduceDamageByPercentage(float Damage, float Percentage);

    // Plays an impact effect at a given location.
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayImpactEffect(const FImpactEffect& effect, const FVector& impactLocation, class AActor* instigator, const UObject* WorldContextObject);

    // Plays an action effect for a given character.
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayActionEffect(const FActionEffect& effect, class ACharacter* instigator, const UObject* WorldContextObject);

    // Plays an action effect locally for a given character.
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayActionEffectLocally(const FActionEffect& effect, class ACharacter* instigator, const UObject* WorldContextObject);

    // Plays a footstep effect for a given pawn and foot bone.
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayFootstepEffect(class APawn* instigator, FName footBone, const UObject* WorldContextObject);

    // Retrieves the current battle type from the game world.
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static EBattleType GetBattleType(const UObject* WorldContextObject);

    // Determines if two teams are enemies.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool AreEnemyTeams(const UObject* WorldContextObject, ETeam teamA, ETeam teamB);

    // Attempts to retrieve the team configuration settings.
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static bool TryGetTeamsConfig(const UObject* WorldContextObject, TMap<ETeam, FTeamInfo>& outConfig);

    // Calculates the distance between two actors.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float CalculateDistanceBetweenActors(const AActor* characterA, const AActor* characterB);

    // Computes a point at a specified distance and direction from an actor.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static FVector GetPointAtDirectionAndDistanceFromActor(const AActor* targetActor, const FVector& direction, float distance, bool bShowDebug = false);

    // Computes the distance between two characters, considering their extents.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float CalculateDistanceBetweenCharactersExtents(const ACharacter* characterA, const ACharacter* characterB);

    // Retrieves the extent (bounding box size) of a character.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float GetCharacterExtent(const ACharacter* characterA);

    // Calculates the distance from a character to a point, considering the character's extent.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float GetCharacterDistantToPointConsideringExtent(const ACharacter* characterA, const FVector& point);

    // Computes the angle between two vectors.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float CalculateAngleBetweenVectors(const FVector& vectorA, const FVector& vectorB);

    // Retrieves the ACF HUD.
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFHUD* GetACFHUD(const UObject* WorldContextObject);

    // Retrieves the local ACF player controller.
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFPlayerController* GetLocalACFPlayerController(const UObject* WorldContextObject);

    // Retrieves the ACF game mode (server-only).
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFGameMode* GetACFGameMode(const UObject* WorldContextObject);

    // Retrieves the ACF game state.
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFGameState* GetACFGameState(const UObject* WorldContextObject);

    // Retrieves the local ACF player character.
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFCharacter* GetLocalACFPlayerCharacter(const UObject* WorldContextObject);

    // Retrieves the ACF team manager component.
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class UACFTeamManagerComponent* GetACFTeamManager(const UObject* WorldContextObject);

    // Determines if an action should be executed based on action chances and character stats.
    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static bool ShouldExecuteAction(const FActionChances& action, const AACFCharacter* characterOwner);

    // Retrieves character attributes.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool GetCharacterAttributes(const TSubclassOf<class AACFCharacter>& characterClass, FAttributesSet& outData, bool bInitialize = true);

    // Retrieves the character's statistics component.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static UARSStatisticsComponent* GetCharacterStatisticComp(const TSubclassOf<class AACFCharacter>& characterClass, bool bInitialize = true);

    // Retrieves character defaults.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static AACFCharacter* GetCharacterDefaults(const TSubclassOf<class AACFCharacter>& characterClass);

    // Retrieves character starting items.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool GetCharacterStartingItems(const TSubclassOf<class AACFCharacter>& characterClass, TArray<FStartingItem>& outItems);

    // Retrieves a character's name.
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool GetCharacterName(const TSubclassOf<class AACFCharacter>& characterClass, FText& outName);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static AActor* FindNearestActorOfClass(const UObject* WorldContextObject, TSubclassOf<class AActor> actorToFind, AActor* origin);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static int32 ExtractIndexWithProbability(const TArray<float>& weights);

    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static bool IsShippingBuild();
        
    UFUNCTION(BlueprintPure, Category = ACFLibrary)
	static bool IsEditor();

};

  