#include "Misc/AutomationTest.h"

#include "Misc/Paths.h"
#include "TTAMicPlayer.h"
#include "TTAMicRecorder.h"

#if WITH_DEV_AUTOMATION_TESTS

// Mic sync fixture (plan req 10): a click every second captured by a mic whose
// clock runs 0.5% fast, delivered in irregular batches stamped with replay time,
// saved to WAV and played back. Every click must land within 50 ms of its
// replay time; a missing file must play silent.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAAudioMicSyncTest, "TTA.Audio.MicSync",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTAAudioMicSyncTest::RunTest(const FString& Parameters)
{
	UTTAMicRecorder* Recorder = NewObject<UTTAMicRecorder>();
	const double MicRate = tta::kMicSampleRate * 1.005;
	const double Seconds = 20.0;
	int64 MicSample = 0;
	int32 Batch = 0;
	TArray<int16> Pending;
	while (MicSample / MicRate < Seconds)
	{
		for (int32 i = 0; i < 320; ++i, ++MicSample)
		{
			const double T = MicSample / MicRate;
			Pending.Add(T >= 1.0 && T - FMath::FloorToDouble(T) < 0.005 ? 20000 : 0);
		}
		if (++Batch % (1 + Batch % 4) == 0)
		{
			Recorder->AppendCaptured(Pending, MicSample / MicRate * 1000.0);
			Pending.Reset();
		}
	}

	const FString Path = FPaths::AutomationTransientDir() / TEXT("MicSync.wav");
	if (!TestTrue(TEXT("saved"), Recorder->Save(Path))) return false;

	UTTAMicPlayer* Player = NewObject<UTTAMicPlayer>();
	if (!TestTrue(TEXT("loaded"), Player->Load(Path))) return false;
	double WorstMs = 0.0;
	for (int32 Second = 1; Second < static_cast<int32>(Seconds); ++Second)
	{
		const TOptional<double> Onset = [&]() -> TOptional<double>
		{
			const auto Found = Player->GetTrack().onsetMs(Second * 1000.0 - 400.0, 10000);
			return Found ? TOptional<double>(*Found) : TOptional<double>();
		}();
		if (!TestTrue(FString::Printf(TEXT("click %d found"), Second), Onset.IsSet())) return false;
		WorstMs = FMath::Max(WorstMs, FMath::Abs(Onset.GetValue() - Second * 1000.0));
		TestEqual(TEXT("ReadAt matches the click"), Player->ReadAt(Onset.GetValue() / 1000.0, 1)[0], int16(20000));
	}
	AddInfo(FString::Printf(TEXT("worst offset %.1f ms"), WorstMs));
	TestTrue(TEXT("offset < 50 ms"), WorstMs < 50.0);

	UTTAMicPlayer* Missing = NewObject<UTTAMicPlayer>();
	AddExpectedError(TEXT("No mic track"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("missing file reports silent"), Missing->Load(FPaths::AutomationTransientDir() / TEXT("NoSuch.wav")));
	bool bSilent = true;
	for (const int16 Sample : Missing->ReadAt(1.0, 256)) bSilent &= Sample == 0;
	TestTrue(TEXT("missing file plays silence"), bSilent);
	return true;
}

#endif
