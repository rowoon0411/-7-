// CXPlayerState.cpp

#include "CXPlayerState.h"
#include "CXGameState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ACXPlayerState::ACXPlayerState()
{
	bReplicates = true;
}

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(ThisClass, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, BonusMaxSpeed, COND_OwnerOnly);
}

int32 ACXPlayerState::GetCoins() const
{
	return Coins;
}

float ACXPlayerState::GetBonusMaxSpeed() const
{
	return BonusMaxSpeed;
}

FString ACXPlayerState::GetPlayerStateInfo() const
{
	FString InfoString = FString::Printf(TEXT("Coins: %d\nBonusMaxSpeed: %.2f"), Coins, BonusMaxSpeed);

	//return InfoString;
	const UWorld* World = GetWorld();
	const ACXGameState* CXGameState = IsValid(World)
		? World->GetGameState<ACXGameState>()
		: nullptr;

	if (IsValid(CXGameState) == false)
	{
		return InfoString;
	}

	switch (CXGameState->GetGamePhase())
	{
	case ECXGamePhase::Waiting:
		return FString::Printf(TEXT("Waiting...\nCoins: %d\nBonusMaxSpeed: %.2f"), Coins, BonusMaxSpeed);

	case ECXGamePhase::Playing:
		return FString::Printf(TEXT("Time: %d\nCoins: %d\nBonusMaxSpeed: %.2f"), CXGameState->GetRemainingTime(), Coins, BonusMaxSpeed);

	case ECXGamePhase::Finished:
		if (CXGameState->IsWinner(this))
		{
			return FString::Printf(TEXT("You win!\nCoins: %d\nBonusMaxSpeed: %.2f"), Coins, BonusMaxSpeed);
		}
		return FString::Printf(TEXT("You lose...\nCoins: %d\nBonusMaxSpeed: %.2f"), Coins, BonusMaxSpeed);

	default:
		return InfoString;
	}

	return InfoString;
}

void ACXPlayerState::AddCoins(int32 Amount)
{
	if (HasAuthority() == false || Amount <= 0)
	{
		return;
	}

	Coins += Amount;
}

bool ACXPlayerState::TryRemoveCoins(int32 Amount)
{
	if (HasAuthority() == false || Amount <= 0 || Coins < Amount)
	{
		return false;
	}

	Coins -= Amount;
	return true;
}

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (HasAuthority() == false || Amount <= 0.0f)
	{
		return;
	}
	
	BonusMaxSpeed += Amount;
}
