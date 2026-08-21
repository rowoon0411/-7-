// CXGameState.cpp

#include "CXGameState.h"
#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, GamePhase);
	DOREPLIFETIME(ThisClass, RemainingTime);
	DOREPLIFETIME(ThisClass, Winners);
}

ECXGamePhase ACXGameState::GetGamePhase() const
{
	return GamePhase;
}

int32 ACXGameState::GetRemainingTime() const
{
	return RemainingTime;
}

bool ACXGameState::IsWinner(const ACXPlayerState* PlayerState) const
{
	return IsValid(PlayerState) && Winners.Contains(PlayerState);
}

void ACXGameState::SetGamePhase(ECXGamePhase NewGamePhase)
{
	if (HasAuthority())
	{
		GamePhase = NewGamePhase;
	}
}

void ACXGameState::SetRemainingTime(int32 NewRemainingTime)
{
	if (HasAuthority())
	{
		RemainingTime = FMath::Max(0, NewRemainingTime);
	}
}

void ACXGameState::SetWinners(const TArray<ACXPlayerState*>& NewWinners)
{
	if (HasAuthority() == false)
	{
		return;
	}

	Winners.Reset();

	for (ACXPlayerState* Winner : NewWinners)
	{
		if (IsValid(Winner))
		{
			Winners.Add(Winner);
		}
	}
}
