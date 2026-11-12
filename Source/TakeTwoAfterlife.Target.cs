using UnrealBuildTool;

public class TakeTwoAfterlifeTarget : TargetRules
{
	public TakeTwoAfterlifeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "TakeTwoAfterlife", "TTACore" });
	}
}
