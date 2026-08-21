// CXItemSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"

UCLASS()
class CX_API ACXItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACXItemSpawner();

protected:
	virtual void BeginPlay() override;

	void SpawnItem();

	UFUNCTION()
	void OnSpawnedItemDestroyed(AActor* DestroyedActor);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Spawner)
	TSubclassOf<AActor> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawner)
	float RespawnDelay = 2.0f;

private:
	UPROPERTY()
	TObjectPtr<AActor> SpawnedItem;

	FTimerHandle RespawnTimerHandle;

};
