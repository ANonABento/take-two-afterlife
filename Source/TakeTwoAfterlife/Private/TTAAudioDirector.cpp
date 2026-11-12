#include "TTAAudioDirector.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAAudio, Log, All);

USoundBase* UTTAAudioDirector::LoadCue(const tta::AudioCue& Cue)
{
	const FString Path = UTF8_TO_TCHAR(tta::assetPath(Cue).c_str());
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path);
	if (!Sound) UE_LOG(LogTTAAudio, Error, TEXT("Missing audio asset %s"), *Path);
	return Sound;
}

void UTTAAudioDirector::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (ATTAGameState* State = InWorld.GetGameState<ATTAGameState>())
	{
		State->OnPhaseChangedNative.AddUObject(this, &UTTAAudioDirector::OnPhaseChanged);
		PlayPhase(State->GetPhase());
	}
}

void UTTAAudioDirector::OnPhaseChanged(ETTAPhase Phase, int32 Round)
{
	PlayPhase(Phase);
}

void UTTAAudioDirector::PlayPhase(ETTAPhase Phase)
{
	const tta::Phase CorePhase = ToCore(Phase);
	if (USoundBase* Track = LoadCue(tta::musicFor(CorePhase)))
	{
		if (Music) Music->FadeOut(MusicFadeSeconds, 0.f);
		Music = UGameplayStatics::CreateSound2D(this, Track, 1.f, 1.f, 0.f, nullptr, true);
		if (Music) Music->FadeIn(MusicFadeSeconds);
	}
	PlayEvent(tta::phaseEvent(CorePhase));
}

void UTTAAudioDirector::PlayEvent(tta::AudioEvent Event)
{
	for (const tta::AudioCue* Cue : tta::cuesFor(Event, tta::AudioKind::Sfx))
	{
		if (USoundBase* Sound = LoadCue(*Cue)) UGameplayStatics::PlaySound2D(this, Sound);
	}
	int32& Count = LinesPlayed.FindOrAdd(static_cast<int32>(Event));
	if (const tta::AudioCue* Line = tta::narratorLine(Event, Count++))
	{
		if (USoundBase* Sound = LoadCue(*Line))
		{
			if (Narrator) Narrator->Stop();
			Narrator = UGameplayStatics::SpawnSound2D(this, Sound);
		}
	}
}
