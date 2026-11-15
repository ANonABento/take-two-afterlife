#include "TTAMicRecorder.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "Interfaces/VoiceCapture.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TTAGameState.h"
#include "VoiceModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAMic, Log, All);

UTTAMicRecorder::UTTAMicRecorder()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

FString UTTAMicRecorder::FilePath(int32 Round, int32 PlayerId)
{
	return FPaths::ProjectSavedDir() / TEXT("Mic") / FString::Printf(TEXT("Round%d_Player%d.wav"), Round, PlayerId);
}

void UTTAMicRecorder::AppendCaptured(TConstArrayView<int16> Pcm, double InReplayMs)
{
	Track.append(std::vector<std::int16_t>(Pcm.begin(), Pcm.end()), InReplayMs - MicLatencyMs);
}

bool UTTAMicRecorder::Save(const FString& Path) const
{
	const std::vector<std::uint8_t> Wav = Track.toWav();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	if (!FFileHelper::SaveArrayToFile(TArrayView<const uint8>(Wav.data(), static_cast<int32>(Wav.size())), *Path))
	{
		UE_LOG(LogTTAMic, Error, TEXT("Could not write mic track %s"), *Path);
		return false;
	}
	return true;
}

void UTTAMicRecorder::Reset()
{
	Track = tta::MicTrack();
}

void UTTAMicRecorder::BeginPlay()
{
	Super::BeginPlay();
	if (ATTAGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ATTAGameState>() : nullptr)
	{
		PhaseHandle = GameState->OnPhaseChangedNative.AddUObject(this, &UTTAMicRecorder::OnPhaseChanged);
		OnPhaseChanged(GameState->GetPhase(), GameState->GetRound()); // spawned mid-Run
	}
}

void UTTAMicRecorder::EndPlay(const EEndPlayReason::Type Reason)
{
	StopAndSave();
	if (ATTAGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ATTAGameState>() : nullptr)
	{
		GameState->OnPhaseChangedNative.Remove(PhaseHandle);
	}
	Super::EndPlay(Reason);
}

void UTTAMicRecorder::OnPhaseChanged(ETTAPhase Phase, int32 Round)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	// AI pawns are locally controlled on the server too; only a local human records (bots have no mic).
	if (Phase == ETTAPhase::Run && Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled())
	{
		StartCapture(Round);
	}
	else if (Phase != ETTAPhase::Run)
	{
		StopAndSave();
	}
}

void UTTAMicRecorder::StartCapture(int32 Round)
{
	if (IsRecording()) return;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
	if (!PlayerState)
	{
		UE_LOG(LogTTAMic, Error, TEXT("No mic track for round %d: runner has no PlayerState"), Round);
		return;
	}
	Reset();
	RecordingRound = Round;
	RecordingPlayerId = PlayerState->GetPlayerId();
	Capture = FVoiceModule::Get().CreateVoiceCapture(FString(), tta::kMicSampleRate, 1);
	if (!Capture.IsValid() || !Capture->Start())
	{
		// Silent fallback: no file is written and playback stays silent.
		UE_LOG(LogTTAMic, Warning, TEXT("No mic capture available; round %d will play back silent"), Round);
		Capture.Reset();
		return;
	}
	SetComponentTickEnabled(true);
}

void UTTAMicRecorder::StopAndSave()
{
	if (!IsRecording()) return;
	Poll();
	Capture->Stop();
	Capture->Shutdown();
	Capture.Reset();
	SetComponentTickEnabled(false);
	Save(FilePath(RecordingRound, RecordingPlayerId));
}

void UTTAMicRecorder::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Poll();
}

void UTTAMicRecorder::Poll()
{
	uint32 Available = 0;
	if (!Capture.IsValid() || Capture->GetCaptureState(Available) != EVoiceCaptureState::Ok || Available == 0) return;
	Buffer.SetNumUninitialized(Available);
	uint32 Read = 0;
	if (Capture->GetVoiceData(Buffer.GetData(), Available, Read) != EVoiceCaptureState::Ok) return;
	AppendCaptured(TConstArrayView<int16>(reinterpret_cast<const int16*>(Buffer.GetData()), Read / 2), ReplayMs());
}

double UTTAMicRecorder::ReplayMs() const
{
	const ATTAGameState* GameState = GetWorld()->GetGameState<ATTAGameState>();
	return GameState ? GameState->GetPhaseElapsedSeconds() * 1000.0 : 0.0;
}
