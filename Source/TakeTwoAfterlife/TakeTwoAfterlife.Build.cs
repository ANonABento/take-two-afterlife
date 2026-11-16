using System.IO;
using UnrealBuildTool;

public class TakeTwoAfterlife : ModuleRules
{
    public TakeTwoAfterlife(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
        bEnableExceptions = true;

        string Root = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
        PublicIncludePaths.Add(Path.Combine(Root, "Core", "include"));
        PrivateIncludePaths.Add(Root);

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DeveloperSettings", "AIModule", "NavigationSystem", "GameplayTasks", "UMG", "Slate", "SlateCore", "MoviePlayer", "EnhancedInput", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils", "NetCore", "Voice", "Json", "ImageWrapper", "ImageCore" });
    }
}
