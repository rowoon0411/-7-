// CXItemSpawner.cpp

#include "CXItemSpawner.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACXItemSpawner::ACXItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = false;
}

void ACXItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnItem();
	}
}

void ACXItemSpawner::SpawnItem()
{
	if (HasAuthority() == false || IsValid(ItemClass) == false)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedItem = World->SpawnActor<AActor>(ItemClass, GetActorTransform(), SpawnParameters);
	if (IsValid(SpawnedItem))
	{
		SpawnedItem->OnDestroyed.AddDynamic(this, &ThisClass::OnSpawnedItemDestroyed);
	}
}

void ACXItemSpawner::OnSpawnedItemDestroyed(AActor* DestroyedActor)
{
	if (HasAuthority() == false || DestroyedActor != SpawnedItem)
	{
		return;
	}

	SpawnedItem = nullptr;

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ThisClass::SpawnItem, RespawnDelay, false);
}
