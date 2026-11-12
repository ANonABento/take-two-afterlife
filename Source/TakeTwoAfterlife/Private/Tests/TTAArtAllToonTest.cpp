#include "Misc/AutomationTest.h"

#include "Components/MeshComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

// Requirement 12: every placed mesh uses an M_TTA_Toon instance. TTA_ArtTest is built by
// Scripts/create_toon_assets.py; the map tasks append Backyard and Heaven to Maps.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAArtAllToonTest, "TTA.Art.AllToon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTAArtAllToonTest::RunTest(const FString& Parameters)
{
	static const TCHAR* Maps[] = {TEXT("/Game/Maps/TTA_ArtTest.TTA_ArtTest")};
	const FString ToonPath = TEXT("/Game/Art/Toon/M_TTA_Toon.M_TTA_Toon");

	for (const TCHAR* MapPath : Maps)
	{
		const UWorld* World = LoadObject<UWorld>(nullptr, MapPath);
		if (!TestNotNull(FString::Printf(TEXT("%s loads"), MapPath), World))
		{
			continue;
		}
		int32 Checked = 0;
		for (const AActor* Actor : World->PersistentLevel->Actors)
		{
			if (!Actor)
			{
				continue;
			}
			TInlineComponentArray<UMeshComponent*> Meshes(Actor);
			for (const UMeshComponent* Mesh : Meshes)
			{
				for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
				{
					const UMaterialInterface* Material = Mesh->GetMaterial(Slot);
					const bool bToonInstance = Material && Material->IsA<UMaterialInstance>()
						&& Material->GetBaseMaterial()->GetPathName() == ToonPath;
					TestTrue(FString::Printf(TEXT("%s slot %d uses an M_TTA_Toon instance (has %s)"),
						*Mesh->GetPathName(), Slot, Material ? *Material->GetPathName() : TEXT("none")), bToonInstance);
					++Checked;
				}
			}
		}
		TestTrue(FString::Printf(TEXT("%s has meshes to check"), MapPath), Checked > 0);
	}
	return true;
}

#endif
