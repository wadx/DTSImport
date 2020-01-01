// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class DTSImport : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }
    private string PluginPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../")); }
    }
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }
    public DTSImport(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivatePCHHeaderFile = "Private/DTSImportPrivatePCH.h"; // since UE 4.21
        bEnableUndefinedIdentifierWarnings = false;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bool isdebug = Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame;
        if (isdebug)
        {
            PublicDefinitions.Add("UE_DEBUG");
        }
        bool isShipping = Target.Configuration == UnrealTargetConfiguration.Shipping;
        if (isShipping)
        {
            PublicDefinitions.Add("UE_SHIPPING");
        }

        PrivateIncludePaths.AddRange(new string[] { "DTSImport/Private" });

        PublicIncludePaths.AddRange(new string[] {});


        PrivateDependencyModuleNames.AddRange(new string[] { "Core" });
        PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine",
			"Slate",
		    "SlateCore",
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "UnrealEd", // for UFactory
        });

        DynamicallyLoadedModuleNames.AddRange(new string[] {});
	}
}
