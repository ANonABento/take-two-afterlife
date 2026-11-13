#include "TTAMicPlayer.h"
#include "GameFramework/Actor.h"

#include "Components/AudioComponent.h"
#include "Misc/FileHelper.h"
#include "Sound/SoundWaveProcedural.h"

#include <stdexcept>

DEFINE_LOG_CATEGORY_STATIC(LogTTAMicPlayer, Log, All);

bool UTTAMicPlayer::Load(const FString& Path)
{
	Track = tta::MicTrack();
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path))
	{
		UE_LOG(LogTTAMicPlayer, Warning, TEXT("No mic track at %s; playing silent"), *Path);
		return false;
	}
	try
	{
		Track = tta::MicTrack::fromWav(std::vector<std::uint8_t>(Bytes.GetData(), Bytes.GetData() + Bytes.Num()));
	}
	catch (const std::invalid_argument& Error)
	{
		UE_LOG(LogTTAMicPlayer, Warning, TEXT("Bad mic track %s (%hs); playing silent"), *Path, Error.what());
		return false;
	}
	return true;
}

TArray<int16> UTTAMicPlayer::ReadAt(double ReplaySeconds, int32 Count) const
{
	const std::vector<std::int16_t> Samples = Track.read(ReplaySeconds * 1000.0, static_cast<std::size_t>(FMath::Max(0, Count)));
	return TArray<int16>(Samples.data(), static_cast<int32>(Samples.size()));
}

void UTTAMicPlayer::PlayFrom(float ReplaySeconds)
{
	if (!Wave)
	{
		Wave = NewObject<USoundWaveProcedural>(this);
		Wave->SetSampleRate(Track.sampleRate());
		Wave->NumChannels = 1;
		Wave->Duration = INDEFINITELY_LOOPING_DURATION;
		Wave->SoundGroup = SOUNDGROUP_Voice;
		Wave->bLooping = false;
		Wave->OnSoundWaveProceduralUnderflow.BindUObject(this, &UTTAMicPlayer::Fill);
	}
	if (!Audio)
	{
		Audio = NewObject<UAudioComponent>(GetOwner());
		Audio->bAutoActivate = false;
		Audio->SetSound(Wave);
		Audio->RegisterComponent();
	}
	Audio->Stop();
	Wave->ResetAudio();
	Cursor = FMath::RoundToInt64(ReplaySeconds * Track.sampleRate());
	Audio->Play();
}

void UTTAMicPlayer::Stop()
{
	if (Audio) Audio->Stop();
}

void UTTAMicPlayer::Fill(USoundWaveProcedural* InWave, int32 SamplesRequired)
{
	const TArray<int16> Samples = ReadAt(static_cast<double>(Cursor) / Track.sampleRate(), SamplesRequired);
	Cursor += SamplesRequired;
	InWave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
}
