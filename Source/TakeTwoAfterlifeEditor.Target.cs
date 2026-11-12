using UnrealBuildTool;

public class TakeTwoAfterlifeEditorTarget : TargetRules
{
	public TakeTwoAfterlifeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "TakeTwoAfterlife", "TTACore" });
	}
}
