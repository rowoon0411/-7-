// CXSpeedBuffItem.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXSpeedBuffItem.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UParticleSystem;
class USoundBase;

UCLASS()
class CX_API ACXSpeedBuffItem : public AActor
{
	GENERATED_BODY()

public:
	ACXSpeedBuffItem();

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACXSpeedBuffItem)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACXSpeedBuffItem)
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACXSpeedBuffItem)
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACXSpeedBuffItem)
	float BonusMaxSpeedAmount = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACXSpeedBuffItem)
	TObjectPtr<UParticleSystem> PickupParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACXSpeedBuffItem)
	TObjectPtr<USoundBase> PickupSound;

private:
	bool bCollected = false;

};