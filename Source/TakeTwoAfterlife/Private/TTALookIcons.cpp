#include "TTALookIcons.h"

#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageCore.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "TextureResource.h"

namespace
{
UTexture2D* TextureFromPng(const FString& Path)
{
	TArray<uint8> Compressed;
	if (!FFileHelper::LoadFileToArray(Compressed, *Path)) return nullptr;
	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Compressed.GetData(), Compressed.Num())) return nullptr;
	// GetRaw is virtual; IImageWrapper::GetRawImage(FImage&) is not exported from ImageWrapper and fails to link.
	TArray64<uint8> Pixels;
	if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Pixels)) return nullptr;
	const int32 Width = static_cast<int32>(Wrapper->GetWidth()), Height = static_cast<int32>(Wrapper->GetHeight());
	const int64 Bytes = static_cast<int64>(Width) * Height * 4;
	if (Width <= 0 || Height <= 0 || Pixels.Num() < Bytes) return nullptr;
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0) return nullptr;
	Texture->SRGB = true;
	Texture->NeverStream = true;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), static_cast<SIZE_T>(Bytes));
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}
}

UTexture2D* TTALoadHatIcon(const char* Name)
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/UI/Looks"),
		FString::Printf(TEXT("T_UI_Hat_%s.png"), UTF8_TO_TCHAR(Name)));
	return TextureFromPng(Path);
}

UTexture2D* TTALoadFaceTexture(const char* Name)
{
	return LoadObject<UTexture2D>(nullptr, *FString::Printf(
		TEXT("/Game/Characters/Faces/T_Face_%s.T_Face_%s"), UTF8_TO_TCHAR(Name), UTF8_TO_TCHAR(Name)));
}
