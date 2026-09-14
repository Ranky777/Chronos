// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Chronos_New : ModuleRules
{
	public Chronos_New(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 模块按子目录组织（Subsystems/、Components/ 等），
		// 把模块根目录加入包含路径后，全模块统一用 "Subsystems/Foo.h" 形式的引用
		PublicIncludePaths.Add(ModuleDirectory);
	
		// AudioMixer：USynthComponent 在 AudioMixer 模块里（程序化音效合成器依赖它）
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AudioMixer" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EnhancedInput",
			"GameplayTags",
			"AIModule",
			"NavigationSystem",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

		// 自动化测试需要 UnrealEd（FAutomationEditorCommonUtils/CreateWorld 场景）
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
