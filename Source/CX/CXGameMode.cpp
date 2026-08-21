// CXGameMode.cpp

#include "CXGameMode.h"
#include "CXGameState.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
	PlayerStateClass = ACXPlayerState::StaticClass();
	GameStateClass = ACXGameState::StaticClass();
}

void ACXGameMode::BeginPlay()
{
	Super::BeginPlay();

	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState))
	{
		CXGameState->SetGamePhase(ECXGamePhase::Waiting);
		CXGameState->SetRemainingTime(WaitingDuration);
	}
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (bWaitingCountdownStarted == false)
	{
		StartWaitingCountdown();
	}
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(AvailablePlayerStarts.IsEmpty() == false, TEXT("There is no available Vehicle PlayerStart."));

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

void ACXGameMode::StartWaitingCountdown()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	bWaitingCountdownStarted = true;

	CXGameState->SetGamePhase(ECXGamePhase::Waiting);
	CXGameState->SetRemainingTime(WaitingDuration);

	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ThisClass::HandleWaitingTimerTick, 1.0f, true);
}

void ACXGameMode::HandleWaitingTimerTick()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);

	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		StartPlayingPhase();
	}
}

void ACXGameMode::StartPlayingPhase()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	CXGameState->SetGamePhase(ECXGamePhase::Playing);
	CXGameState->SetRemainingTime(PlayingDuration);

	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ThisClass::HandlePlayingTimerTick, 1.0f, true);
}

void ACXGameMode::HandlePlayingTimerTick()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;

	CXGameState->SetRemainingTime(NewRemainingTime);

	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		FinishGame();
	}
}

void ACXGameMode::FinishGame()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	int32 HighestCoins = TNumericLimits<int32>::Lowest();
	TArray<ACXPlayerState*> Winners;

	for (APlayerState* PlayerState : CXGameState->PlayerArray)
	{
		ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PlayerState);
		if (IsValid(CXPlayerState) == false)
		{
			continue;
		}

		const int32 PlayerCoins = CXPlayerState->GetCoins();
		if (HighestCoins < PlayerCoins)
		{
			HighestCoins = PlayerCoins;
			Winners.Reset();
			Winners.Add(CXPlayerState);
		}
		else if (PlayerCoins == HighestCoins)
		{
			Winners.Add(CXPlayerState);
		}
	}

	CXGameState->SetWinners(Winners);
	CXGameState->SetRemainingTime(0);
	CXGameState->SetGamePhase(ECXGamePhase::Finished);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}
