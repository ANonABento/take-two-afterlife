#include "Misc/AutomationTest.h"

#include "tta/Premiere.h"
#include "tta/Ratings.h"
#include "tta/Replay.h"

#if WITH_DEV_AUTOMATION_TESTS

// Builder attribution: a cut's builder is the owner of the trap that wiped out the
// victim, not the editor, so Evil points go to the trap owner; no trap death, no builder.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTACutCreditsTest, "TTA.Premiere.CutCredits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTACutCreditsTest::RunTest(const FString& Parameters)
{
	tta::Replay Replay({0, 1, 2});
	Replay.addEvent({tta::ReplayEventType::Wipeout, 1200, 1, 7, 2});
	const tta::Cut Cut = tta::cutCredits(0, 1, Replay);
	TestEqual(TEXT("editor stays the director"), Cut.editor, 0);
	TestEqual(TEXT("builder is the trap owner"), Cut.builder, 2);

	tta::RatingBook Book;
	Book.openCut(1, Cut);
	Book.rate(1, 1, 3, 5);
	const std::map<int, double> Points = Book.ratingPoints();
	TestEqual(TEXT("editor gets Funny only"), Points.at(0), 3.0);
	TestEqual(TEXT("trap owner gets Evil"), Points.at(2), 5.0);

	tta::Replay Smite({0, 1});
	Smite.addEvent({tta::ReplayEventType::Smite, 900, 1});
	TestEqual(TEXT("smite has no builder"), tta::cutCredits(0, 1, Smite).builder, tta::kNoBuilder);
	return true;
}

#endif
