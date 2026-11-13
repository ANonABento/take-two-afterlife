#include "TTAAudioDirector.h"
#include "Engine/World.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAAudio, Log, All);

namespace
{
	// One session = from the first game world's director to the last one going away (a PIE run,
	// or the packaged game's lifetime). PIE clients share it, so each asset warns once per run.
	tta::MissingAssetLog& MissingAudio()
	{
		static tta::MissingAssetLog Log;
		return Log;
	}
	int32 LiveGameDirectors = 0;
}

USoundBase* UTTAAudioDirector::LoadCue(const tta::AudioCue& Cue)
{
	const std::string Key = tta::assetPath(Cue);
	if (MissingAudio().isKnownMissing(Key)) return nullptr;

	const FString Path = UTF8_TO_TCHAR(Key.c_str());
	USoundBase* Sound = nullptr;
	// Check the package first: LoadObject on a missing package logs its own warning on every call.
	if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
	{
		Sound = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!Sound && MissingAudio().reportMissing(Key))
	{
		UE_LOG(LogTTAAudio, Warning,
			TEXT("Missing audio asset %s; skipping it for the rest of this session (Scripts/setup_content.ps1 creates silent placeholders)"),
			*Path);
	}
	return Sound;
}

void UTTAAudioDirector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const UWorld* World = GetWorld();
	bCountsAsGameWorld = World && World->IsGameWorld();
	if (bCountsAsGameWorld && LiveGameDirectors++ == 0) MissingAudio().reset();
}

void UTTAAudioDirector::Deinitialize()
{
	if (bCountsAsGameWorld) --LiveGameDirectors;
	bCountsAsGameWorld = false;
	Super::Deinitialize();
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
