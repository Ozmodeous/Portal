// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class PortalTarget : TargetRules
{
    public PortalTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

        ExtraModuleNames.AddRange(new string[] { "Portal" });

        // Network optimization
        bWithServerCode = true;
    }
}