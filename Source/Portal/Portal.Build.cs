// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

using UnrealBuildTool;

public class Portal : ModuleRules
{
    public Portal(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

        // Core UE5 modules
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "NavigationSystem",
            "AIModule",
            "GameplayTasks",
            "UMG",
            "Slate",
            "SlateCore",
            "GameplayTags"
        });

        // Networking
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "Sockets",
            "Networking"
        });

        // ACF Ultimate - Core modules only
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "AscentCombatFramework",
            "AscentCoreInterfaces",
            "AIFramework",
            "AdvancedRPGSystem",
            "ActionsSystem"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "NetCore",
            "EngineSettings",
            "DeveloperSettings",
            "PacketHandler",
            "ReliabilityHandlerComponent"
        });

        // Platform Steam integration
        if (Target.Platform == UnrealTargetPlatform.Win64 ||
            Target.Platform == UnrealTargetPlatform.Linux ||
            Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicDependencyModuleNames.Add("OnlineSubsystemSteam");
        }

        // UE 5.5 optimizations
        bUseUnity = true;
        MinFilesUsingPrecompiledHeaderOverride = 1;
        bEnableExceptions = false;
    }
}