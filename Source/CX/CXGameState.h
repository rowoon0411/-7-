// CXGameState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;

UENUM(BlueprintType)
enum class ECXGamePhase : uint8
{
	Waiting,
	Playing,
	Finished
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ECXGamePhase GetGamePhase() const;

	int32 GetRemainingTime() const;

	bool IsWinner(const ACXPlayerState* PlayerState) const;

	void SetGamePhase(ECXGamePhase NewGamePhase);

	void SetRemainingTime(int32 NewRemainingTime);

	void SetWinners(const TArray<ACXPlayerState*>& NewWinners);

protected:
	UPROPERTY(Replicated)
	ECXGamePhase GamePhase = ECXGamePhase::Waiting;

	UPROPERTY(Replicated)
	int32 RemainingTime = 0;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<ACXPlayerState>> Winners;

};
