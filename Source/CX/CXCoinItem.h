// CXCoinItem.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXCoinItem.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UParticleSystem;
class USoundBase;

UCLASS()
class CX_API ACXCoinItem : public AActor
{
	GENERATED_BODY()

public:
	ACXCoinItem();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCPlayPickupEffects(FVector EffectLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACXCoinItem)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACXCoinItem)
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACXCoinItem)
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACXCoinItem)
	int32 CoinAmount = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACXCoinItem)
	TObjectPtr<UParticleSystem> PickupParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACXCoinItem)
	TObjectPtr<USoundBase> PickupSound;

private:
	bool bCollected = false;

};
