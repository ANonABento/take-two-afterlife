#pragma once

#include "CoreMinimal.h"
#include "tta/EditList.h"

// Reels gallery (plan: cuts autosave locally): Saved/Reels/<name>.ttacut, Core text format.
namespace TTAReels
{
	TAKETWOAFTERLIFE_API FString Dir();
	// Name for one director's cut of one round; saving again overwrites it.
	TAKETWOAFTERLIFE_API FString FileFor(int32 Round, const FString& Director);
	// Logs and returns false on a write error or an unsavable cut.
	TAKETWOAFTERLIFE_API bool Save(const tta::EditList& Cut, const FString& Path);
	// Logs and returns unset for a missing or invalid file.
	TAKETWOAFTERLIFE_API TOptional<tta::EditList> Load(const FString& Path);
	// Full paths, newest first, for the main-menu gallery (task 24).
	TAKETWOAFTERLIFE_API TArray<FString> List();
}
