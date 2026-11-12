#include "TTASessionSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "tta/JoinCode.h"

namespace
{
const FName JoinCodeKey(TEXT("JOINCODE"));

FString CoreToUE(const std::string& S) { return UTF8_TO_TCHAR(S.c_str()); }
}

IOnlineSessionPtr UTTASessionSubsystem::Sessions() const
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	return Subsystem ? Subsystem->GetSessionInterface() : nullptr;
}

void UTTASessionSubsystem::Login()
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	const IOnlineIdentityPtr Identity = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;
	if (!Identity)
	{
		OnLoginComplete.Broadcast(false, TEXT("No online subsystem; use direct IP."));
		return;
	}
	Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
	LoginHandle = Identity->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateWeakLambda(this,
		[this, Identity](int32, bool bOk, const FUniqueNetId&, const FString& Error)
		{
			Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
			OnLoginComplete.Broadcast(bOk, Error);
		}));
	// Dev runs pass -AUTH_TYPE=developer -AUTH_LOGIN=... -AUTH_PASSWORD=..., which AutoLogin reads.
	FString AuthType;
	const bool bStarted = FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType)
		? Identity->AutoLogin(0)
		: Identity->Login(0, FOnlineAccountCredentials(TEXT("accountportal"), FString(), FString()));
	if (!bStarted)
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
		OnLoginComplete.Broadcast(false, TEXT("Epic login could not start."));
	}
}

void UTTASessionSubsystem::HostLobby()
{
	const IOnlineSessionPtr S = Sessions();
	if (!S)
	{
		OnHostComplete.Broadcast(false, TEXT("No online subsystem; use direct IP."));
		return;
	}
	JoinCode = CoreToUE(tta::makeJoinCode(FDateTime::UtcNow().GetTicks() ^ FPlatformTime::Cycles64()));

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = MaxPlayers;
	Settings.bShouldAdvertise = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bIsLANMatch = false;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bUseLobbiesVoiceChatIfAvailable = true;
	Settings.SessionIdOverride = JoinCode;
	Settings.Set(JoinCodeKey, JoinCode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	S->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	CreateHandle = S->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UTTASessionSubsystem::HandleCreate));
	if (!S->CreateSession(0, NAME_GameSession, Settings))
	{
		HandleCreate(NAME_GameSession, false);
	}
}

void UTTASessionSubsystem::HandleCreate(FName, bool bOk)
{
	Sessions()->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	if (!bOk)
	{
		JoinCode.Reset();
		OnHostComplete.Broadcast(false, TEXT("Could not create the EOS lobby."));
		return;
	}
	OnHostComplete.Broadcast(true, JoinCode);
	GetWorld()->ServerTravel(LobbyMap + TEXT("?listen"));
}

void UTTASessionSubsystem::JoinByCode(const FString& TypedCode)
{
	const std::optional<std::string> Code = tta::normalizeJoinCode(TCHAR_TO_UTF8(*TypedCode));
	if (!Code)
	{
		OnJoinComplete.Broadcast(false, TEXT("Codes are 6 letters."));
		return;
	}
	if (!Sessions())
	{
		OnJoinComplete.Broadcast(false, TEXT("No online subsystem; use direct IP."));
		return;
	}
	PendingCode = CoreToUE(*Code);
	FindAttemptsLeft = FindAttempts;
	FindByCode();
}

void UTTASessionSubsystem::FindByCode()
{
	const IOnlineSessionPtr S = Sessions();
	--FindAttemptsLeft;
	Search = MakeShared<FOnlineSessionSearch>();
	Search->MaxSearchResults = 10;
	Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	Search->QuerySettings.Set(JoinCodeKey, PendingCode, EOnlineComparisonOp::Equals);
	S->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	FindHandle = S->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UTTASessionSubsystem::HandleFind));
	if (!S->FindSessions(0, Search.ToSharedRef()))
	{
		HandleFind(false);
	}
}

void UTTASessionSubsystem::HandleFind(bool bOk)
{
	const IOnlineSessionPtr S = Sessions();
	S->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	const FOnlineSessionSearchResult* Found = nullptr;
	for (const FOnlineSessionSearchResult& Result : Search->SearchResults)
	{
		FString Code;
		if (Result.Session.SessionSettings.Get(JoinCodeKey, Code) && Code == PendingCode)
		{
			Found = &Result;
			break;
		}
	}
	if (!Found)
	{
		if (FindAttemptsLeft > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(RetryTimer,
				FTimerDelegate::CreateUObject(this, &UTTASessionSubsystem::FindByCode), FindRetrySeconds, false);
			return;
		}
		FailJoin(bOk ? TEXT("No lobby with that code.") : TEXT("Lobby search failed."));
		return;
	}
	S->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	JoinHandle = S->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UTTASessionSubsystem::HandleJoin));
	if (!S->JoinSession(0, NAME_GameSession, *Found))
	{
		HandleJoin(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UTTASessionSubsystem::HandleJoin(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	const IOnlineSessionPtr S = Sessions();
	S->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	FString Address;
	if (Result != EOnJoinSessionCompleteResult::Success || !S->GetResolvedConnectString(SessionName, Address))
	{
		FailJoin(FString::Printf(TEXT("Join failed (%s)."), LexToString(Result)));
		return;
	}
	OnJoinComplete.Broadcast(true, PendingCode);
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->ClientTravel(Address, TRAVEL_Absolute);
}

void UTTASessionSubsystem::FailJoin(const FString& Message)
{
	UE_LOG(LogTemp, Warning, TEXT("TTA join by code %s: %s"), *PendingCode, *Message);
	OnJoinComplete.Broadcast(false, Message);
}

void UTTASessionSubsystem::HostDirect()
{
	// Dev fallback: listen server on the IP net driver's port (same LAN or a port forward).
	GetWorld()->ServerTravel(LobbyMap + TEXT("?listen"));
}

bool UTTASessionSubsystem::JoinDirect(const FString& TypedAddress)
{
	const std::optional<tta::DirectAddress> Address = tta::parseDirectAddress(TCHAR_TO_UTF8(*TypedAddress));
	if (!Address)
	{
		OnJoinComplete.Broadcast(false, TEXT("Type an address like 192.168.1.5 or 192.168.1.5:7777."));
		return false;
	}
	const FString Url = FString::Printf(TEXT("%s:%d"), *CoreToUE(Address->host), Address->port);
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->ClientTravel(Url, TRAVEL_Absolute);
	return true;
}
