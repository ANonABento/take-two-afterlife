using System.IO;
using UnrealBuildTool;

// Wraps the engine-free C++17 library in /Core. Core/ stays the single source of truth:
// Private/Core*.cpp only #include the files from Core/src, so nothing is copied.
public class TTACore : ModuleRules
{
	public TTACore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		bUseUnity = false; // Core/src files have file-local helpers that would clash in one unity file
		bEnableExceptions = true; // Core reports bad input with std::invalid_argument
		PublicDependencyModuleNames.Add("Core");
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "..", "Core", "include"));
	}
}
