// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

using UnrealBuildTool;

/// <summary>
/// Portal Game Module - UE 5.5.4 Build Configuration
/// Optimized for Ascent Combat Framework Ultimate Plugin Integration
/// </summary>
public class Portal : ModuleRules
{
    public Portal(ReadOnlyTargetRules Target) : base(Target)
    {
        // Configure PCH usage for optimal compilation performance in UE 5.5
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

        // Core Unreal Engine 5.5 Modules - Essential Foundation
        PublicDependencyModuleNames.AddRange(new string[]
        {
            // Core Engine Systems
            "Core",                 // Essential C++ types and utilities
            "CoreUObject",          // UObject framework and reflection
            "Engine",               // Core engine functionality
            "InputCore",            // Input system foundation
            "EnhancedInput",        // UE 5.x enhanced input system

            // AI and Navigation Systems (Essential for ACF Integration)
            "NavigationSystem",     // AI pathfinding and navigation mesh
            "AIModule",             // AI controllers and behavior trees
            "GameplayTasks",        // Asynchronous task execution system

            // UI and Presentation Systems
            "UMG",                  // User interface widget system
            "Slate",                // Low-level UI framework
            "SlateCore",            // Core Slate components

            // Gameplay Framework
            "GameplayTags"          // Structured tag system for game logic
        });

        // Networking and Multiplayer Infrastructure
        PublicDependencyModuleNames.AddRange(new string[]
        {
            // Online Subsystem Integration
            "OnlineSubsystem",      // Platform-agnostic online services
            "OnlineSubsystemUtils", // Utility functions for online systems
            "Sockets",              // Low-level socket networking
            "Networking"            // High-level networking abstractions
        });

        // Ascent Combat Framework Ultimate - Core Module Integration
        // Configured for optimal ACF plugin compatibility and performance
        PublicDependencyModuleNames.AddRange(new string[]
        {
            // Primary ACF Framework Modules
            "AscentCombatFramework",    // Core combat system and mechanics
            "AscentCoreInterfaces",     // Foundational interfaces and types
            "AIFramework",              // Advanced AI behavior systems
            "AdvancedRPGSystem",        // Character progression and stats
            "ActionsSystem"             // Action-based gameplay mechanics
        });

        // Private Dependencies - Internal Implementation Details
        // These modules are used internally but not exposed in public headers
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            // Core Engine Internals
            "NetCore",                      // Core networking implementation
            "EngineSettings",               // Engine configuration system
            "DeveloperSettings",            // Development-time settings
            "PacketHandler",                // Network packet processing
            "ReliabilityHandlerComponent"   // Network reliability management
        });

        // Platform-Specific Integration - Steam Ecosystem
        // Conditional compilation based on target platform capabilities
        if (Target.Platform == UnrealTargetPlatform.Win64 ||
            Target.Platform == UnrealTargetPlatform.Linux ||
            Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicDependencyModuleNames.Add("OnlineSubsystemSteam");
        }

        // UE 5.5 Performance Optimization Settings
        // Configured for optimal compilation speed and runtime performance
        bUseUnity = true;                               // Enable unity builds for faster compilation
        MinFilesUsingPrecompiledHeaderOverride = 1;     // Aggressive PCH usage
        bEnableExceptions = false;                      // Disable C++ exceptions for performance

        // Advanced Build Configuration for ACF Integration
        bUseRTTI = false;                              // Disable RTTI for performance optimization
        UndefinedIdentifierWarningLevel = WarningLevel.Error; // Enhanced error detection (UE 5.5)

        // Memory and Performance Optimizations
        OptimizeCode = CodeOptimization.Default;        // Use default optimization level
        bLegacyPublicIncludePaths = false;             // Use modern include path structure

        // Development and Debugging Configuration
        if (Target.Configuration == UnrealTargetConfiguration.Debug ||
            Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            // Enhanced debugging support for development builds
            PublicDefinitions.Add("PORTAL_DEBUG_ENABLED=1");
            PublicDefinitions.Add("ACF_DEBUG_INTEGRATION=1");
        }
        else
        {
            // Production optimizations for shipping builds
            PublicDefinitions.Add("PORTAL_DEBUG_ENABLED=0");
            PublicDefinitions.Add("ACF_DEBUG_INTEGRATION=0");
        }

        // Ascent Combat Framework Integration Definitions
        // These definitions ensure proper ACF feature compatibility
        PublicDefinitions.AddRange(new string[]
        {
            "ACF_ULTIMATE_INTEGRATION=1",       // Enable full ACF Ultimate features
            "PORTAL_ACF_VERSION=55",            // ACF version compatibility marker
            "UE_5_5_COMPATIBILITY=1"           // UE 5.5 feature compatibility
        });

        // Include Path Optimization for ACF Integration
        // Ensures proper header resolution for ACF plugin dependencies
        PublicIncludePaths.AddRange(new string[]
        {
            // No additional include paths needed - using module dependencies
            // All ACF headers are resolved through module system
        });

        // Private Include Path Configuration
        // Note: Private directory will be created automatically during build process
        // PrivateIncludePaths.AddRange(new string[]
        // {
        //     "Portal/Private"
        // });

        // Dynamic Module Loading Configuration
        // Modules that are loaded at runtime rather than link-time
        DynamicallyLoadedModuleNames.AddRange(new string[]
        {
            // Steam integration loaded dynamically for cross-platform compatibility
            // No dynamic modules required for current configuration
        });

        // Editor-Specific Configuration
        if (Target.bBuildEditor)
        {
            // Editor-specific functionality
            PublicDefinitions.Add("PORTAL_EDITOR_BUILD=1");

            // Additional editor modules for development workflow
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "ToolMenus",           // Editor menu system
                "EditorStyle",         // Editor visual styling
                "EditorWidgets"        // Editor UI components
            });
        }
        else
        {
            // Runtime-only build configuration
            PublicDefinitions.Add("PORTAL_EDITOR_BUILD=0");
        }

        // Memory Allocation and Performance Settings
        // Optimized for ACF Ultimate's memory usage patterns
        bEnableBufferSecurityChecks = true;            // Enable security checks

        // Module Validation and Error Prevention
        // Ensures clean module dependencies without circular references
        bValidateCircularDependencies = true;
    }
}