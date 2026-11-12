#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "TTASessionSubsystem.generated.h"

class FOnlineSessionSearch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTTASessionResultSignature, bool, bOk, const FString&, Message);

// EOS lobby flow (plan req 9, choomfie-library/ref/eos-ue5.md):
// Login (Account Portal, or -AUTH_TYPE=developer via AutoLogin) -> HostLobby
// (lobby with a 6-letter code, listen server) / JoinByCode (search by code, join,
// ClientTravel). HostDirect/JoinDirect are the direct-IP dev fallback.
UCLASS(Config = Game)
class TAKETWOAFTERLIFE_API UTTASessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable) void Login();
	UFUNCTION(BlueprintCallable) void HostLobby();
	UFUNCTION(BlueprintCallable) void JoinByCode(const FString& TypedCode);
	UFUNCTION(BlueprintCallable) void HostDirect();
	UFUNCTION(BlueprintCallable) bool JoinDirect(const FString& TypedAddress);

	// The code the host shows in the lobby; empty until HostLobby succeeds.
	UFUNCTION(BlueprintPure) FString GetJoinCode() const { return JoinCode; }

	UPROPERTY(BlueprintAssignable) FTTASessionResultSignature OnLoginComplete;
	// Message is the join code on success.
	UPROPERTY(BlueprintAssignable) FTTASessionResultSignature OnHostComplete;
	UPROPERTY(BlueprintAssignable) FTTASessionResultSignature OnJoinComplete;

private:
	IOnlineSessionPtr Sessions() const;
	void FindByCode();
	void HandleCreate(FName SessionName, bool bOk);
	void HandleFind(bool bOk);
	void HandleJoin(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void FailJoin(const FString& Message);

	// Map the host travels to with ?listen (menus land in task 24).
	UPROPERTY(Config) FString LobbyMap = TEXT("/Game/Maps/Title");
	UPROPERTY(Config) int32 MaxPlayers = 4;
	// New lobbies take up to 3 s to be searchable, so a miss is retried.
	UPROPERTY(Config) int32 FindAttempts = 4;
	UPROPERTY(Config) float FindRetrySeconds = 1.5f;

	FString JoinCode;
	FString PendingCode;
	int32 FindAttemptsLeft = 0;
	TSharedPtr<FOnlineSessionSearch> Search;
	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle LoginHandle;
	FTimerHandle RetryTimer;
};
