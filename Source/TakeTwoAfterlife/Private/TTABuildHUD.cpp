#include "TTABuildHUD.h"

#include "Engine/Engine.h"
#include "TTABuildPawn.h"

void ATTABuildHUD::DrawHUD()
{
	Super::DrawHUD();
	const ATTABuildPawn* Pawn = Cast<ATTABuildPawn>(GetOwningPawn());
	if (!Pawn) return;

	const int32 Me = ATTAPlacementMarker::PlayerIdOf(Pawn);
	const tta::Layout Own = Pawn->OwnLayout();
	TArray<TPair<FString, FLinearColor>> Lines;
	Lines.Emplace(FString::Printf(TEXT("BUDGET %d / %d"), Own.remaining(Me), tta::kBudget), FLinearColor::White);
	Lines.Emplace(FString::Printf(TEXT("CAMERAS %d / %d   fixed %d  pan %d  follow %d  trap-cam %d"), Own.cameraCount(Me), tta::kCameraCap,
		Own.cameraCount(Me, tta::CameraType::Fixed), Own.cameraCount(Me, tta::CameraType::Pan),
		Own.cameraCount(Me, tta::CameraType::Follow), Own.cameraCount(Me, tta::CameraType::TrapCam)), FLinearColor::White);
	const bool bGhostOk = Pawn->GetGhostOutcome() == ETTAPlaceOutcome::Ok;
	Lines.Emplace(FString::Printf(TEXT("%s   [wheel] item  [R] rotate  [LMB] place  [RMB] remove  [F] select"),
		*TTABuildItem::Label(Pawn->GetSelectedItem())), FLinearColor::White);
	Lines.Emplace(bGhostOk ? FString(TEXT("Can place here")) : UEnum::GetDisplayValueAsText(Pawn->GetGhostOutcome()).ToString(),
		bGhostOk ? FLinearColor::Green : FLinearColor::Red);
	if (Pawn->GetLastServerOutcome() != ETTAPlaceOutcome::Ok)
	{
		Lines.Emplace(FString::Printf(TEXT("Rejected: %s"), *UEnum::GetDisplayValueAsText(Pawn->GetLastServerOutcome()).ToString()), FLinearColor::Red);
	}
	if (const ATTAPlacementMarker* Selected = Pawn->FindMarker(Pawn->GetSelectedPlacement()))
	{
		const tta::Placement P = Selected->GetData().ToCore();
		if (!P.isCamera)
		{
			const tta::TrapSpec& Spec = tta::trapSpec(P.trap);
			const TCHAR* Keys[] = {TEXT("[1]-  [2]+"), TEXT("[3]-  [4]+")};
			Lines.Emplace(FString::Printf(TEXT("SELECTED %s #%d"), UTF8_TO_TCHAR(Spec.name), P.id), FLinearColor::Yellow);
			for (int32 i = 0; i < 2; ++i)
			{
				const tta::SettingSpec& S = Spec.settings[static_cast<size_t>(i)];
				Lines.Emplace(FString::Printf(TEXT("%s  %s: %d  (%d-%d)"), Keys[i], UTF8_TO_TCHAR(S.name),
					P.settings[static_cast<size_t>(i)], S.min, S.max), FLinearColor::Yellow);
			}
		}
	}

	float Y = 40.f;
	for (const TPair<FString, FLinearColor>& Line : Lines)
	{
		DrawText(Line.Key, Line.Value, 40.f, Y, GEngine->GetLargeFont());
		Y += 30.f;
	}
}
