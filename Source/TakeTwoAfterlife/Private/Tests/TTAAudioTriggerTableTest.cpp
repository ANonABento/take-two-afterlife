#include "Misc/AutomationTest.h"

#include "Sound/SoundBase.h"
#include "TTAAudioDirector.h"
#include "tta/AudioCues.h"

#if WITH_DEV_AUTOMATION_TESTS

// Narrator + music + SFX (task 25): every audio event in tta::audioCues() maps to an
// imported sound asset, and every phase has its music track and a narrator line.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAAudioTriggerTableTest, "TTA.Audio.TriggerTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FTTAAudioTriggerTableTest::RunTest(const FString& Parameters)
{
	for (int32 E = 0; E < static_cast<int32>(tta::AudioEvent::Count); ++E)
	{
		const auto Event = static_cast<tta::AudioEvent>(E);
		const FString Name = UTF8_TO_TCHAR(tta::audioEventName(Event));
		int32 Cues = 0;
		for (const tta::AudioKind Kind : {tta::AudioKind::Narrator, tta::AudioKind::Music, tta::AudioKind::Sfx})
		{
			for (const tta::AudioCue* Cue : tta::cuesFor(Event, Kind))
			{
				++Cues;
				TestNotNull(Name + TEXT(" -> ") + UTF8_TO_TCHAR(Cue->id), UTTAAudioDirector::LoadCue(*Cue));
			}
		}
		TestTrue(Name + TEXT(" has a cue"), Cues > 0);
	}
	for (int32 P = 0; P <= static_cast<int32>(tta::Phase::Podium); ++P)
	{
		const auto Phase = static_cast<tta::Phase>(P);
		const FString Name = UTF8_TO_TCHAR(tta::phaseName(Phase));
		TestNotNull(Name + TEXT(" music"), UTTAAudioDirector::LoadCue(tta::musicFor(Phase)));
		TestNotNull(Name + TEXT(" narrator line"), tta::narratorLine(tta::phaseEvent(Phase), 0));
	}
	return true;
}

#endif
