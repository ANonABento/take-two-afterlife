#include "TTAReels.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "tta/CutPlayback.h"

#include <stdexcept>

DEFINE_LOG_CATEGORY_STATIC(LogTTAReels, Log, All);

FString TTAReels::Dir()
{
	return FPaths::ProjectSavedDir() / TEXT("Reels");
}

FString TTAReels::FileFor(int32 Round, const FString& Director)
{
	return Dir() / FString::Printf(TEXT("Round%d_%s.ttacut"), Round, *FPaths::MakeValidFileName(Director));
}

bool TTAReels::Save(const tta::EditList& Cut, const FString& Path)
{
	std::string Text;
	try
	{
		Text = tta::saveCut(Cut);
	}
	catch (const std::invalid_argument& Error)
	{
		UE_LOG(LogTTAReels, Error, TEXT("Can't save reel %s: %hs"), *Path, Error.what());
		return false;
	}
	if (!FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Text.c_str()), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTTAReels, Error, TEXT("Can't write reel %s"), *Path);
		return false;
	}
	return true;
}

TOptional<tta::EditList> TTAReels::Load(const FString& Path)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogTTAReels, Error, TEXT("No reel at %s"), *Path);
		return {};
	}
	try
	{
		return tta::loadCut(TCHAR_TO_UTF8(*Text));
	}
	catch (const std::invalid_argument& Error)
	{
		UE_LOG(LogTTAReels, Error, TEXT("Bad reel %s: %hs"), *Path, Error.what());
		return {};
	}
}

TArray<FString> TTAReels::List()
{
	TArray<FString> Names;
	IFileManager::Get().FindFiles(Names, *(Dir() / TEXT("*.ttacut")), true, false);
	TArray<FString> Paths;
	for (const FString& Name : Names) Paths.Add(Dir() / Name);
	Paths.Sort([](const FString& A, const FString& B)
	{
		return IFileManager::Get().GetTimeStamp(*A) > IFileManager::Get().GetTimeStamp(*B);
	});
	return Paths;
}
