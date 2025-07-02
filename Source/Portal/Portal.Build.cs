// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.
using UnrealBuildTool;

public class Portal : ModuleRules
{
    public Portal(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "NavigationSystem",
            "AIModule",
            "UMG",
            "Slate",
            "SlateCore",
            "GameplayTags",
            // Multiplayer modules
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "Sockets",
            "Networking",
            // ACF/ARS Dependencies
            "AscentCoreInterfaces",
            "AscentCombatFramework",
            "AscentTargetingSystem",
            "AdvancedRPGSystem",
            "AIFramework",
            "ActionsSystem"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "NetCore",
            "EngineSettings",
            "DeveloperSettings",
            "PacketHandler",
            "ReliabilityHandlerComponent",
            "ToolMenus",
            // Additional ACF modules that might be needed
            "CollisionsManager",
            "CharacterController",
            "MotionWarping"
        });

        // Platform-specific Steam integration
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDependencyModuleNames.Add("OnlineSubsystemSteam");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicDependencyModuleNames.Add("OnlineSubsystemSteam");
        }
    }
}