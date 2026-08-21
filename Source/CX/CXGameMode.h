// CXGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "CXGameMode.generated.h"

UCLASS(MinimalAPI)
class ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXGameMode();

protected:
	virtual void BeginPlay() override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void StartWaitingCountdown();

	void HandleWaitingTimerTick();

	void StartPlayingPhase();

	void HandlePlayingTimerTick();

	void FinishGame();

private:
	bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);

	UPROPERTY(EditDefaultsOnly, Category = Game)
	int32 WaitingDuration = 5;

	UPROPERTY(EditDefaultsOnly, Category = Game)
	int32 PlayingDuration = 30;

	FTimerHandle GameTimerHandle;

	bool bWaitingCountdownStarted = false;

};



