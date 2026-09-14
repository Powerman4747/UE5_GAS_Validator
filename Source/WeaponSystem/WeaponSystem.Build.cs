// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WeaponSystem : ModuleRules
{
	public WeaponSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});


		PrivateDependencyModuleNames.AddRange(new string[] { "GameplayAbilities", "GameplayTags", "GameplayTasks" });

		PublicIncludePaths.AddRange(new string[] {
			"WeaponSystem",
			"WeaponSystem/Variant_Horror",
			"WeaponSystem/Variant_Horror/UI",
			"WeaponSystem/Variant_Shooter",
			"WeaponSystem/Variant_Shooter/AI",
			"WeaponSystem/Variant_Shooter/UI",
			"WeaponSystem/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
