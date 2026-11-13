#include "TTAWorldLook.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

void UTTAWorldLookLibrary::ApplyWorldLook(const UObject* WorldContext, UMaterialParameterCollection* Collection, ETTAWorld World)
{
	check(Collection);
	const UTTAWorldLookSettings* Settings = GetDefault<UTTAWorldLookSettings>();
	const FTTAWorldLook& Look = World == ETTAWorld::Heaven ? Settings->Heaven : Settings->Course;
	const TPair<FName, float> Values[] = {
		{TEXT("Saturation"), Look.Saturation}, {TEXT("Contrast"), Look.Contrast},
		{TEXT("ShadowFloor"), Look.ShadowFloor}, {TEXT("Bands"), Look.Bands},
		{TEXT("RimStrength"), Look.RimStrength}, {TEXT("Warmth"), Look.Warmth}};
	for (const TPair<FName, float>& Value : Values)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(const_cast<UObject*>(WorldContext), Collection, Value.Key, Value.Value);
	}
}
