// Copyright (c) 2024 Synty Studios Limited. All rights reserved.

using UnrealBuildTool;

public class PolygonScifiWorlds : ModuleRules
{
	public PolygonScifiWorlds(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "GameplayAbilities", "GameplayTags", "GameplayTasks", "UMG", "DeveloperSettings", "MassEntity", "MassSpawner", "MassAIBehavior", "StateTreeModule", "MassCommon" });

		PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "MassActors", "MassSignals", "MassSimulation" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
