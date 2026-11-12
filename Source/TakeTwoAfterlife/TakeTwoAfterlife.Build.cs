using UnrealBuildTool;

public class TakeTwoAfterlife : ModuleRules
{
	public TakeTwoAfterlife(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableExceptions = true;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DeveloperSettings", "TTACore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils", "NetCore", "Voice" });
	}
}
