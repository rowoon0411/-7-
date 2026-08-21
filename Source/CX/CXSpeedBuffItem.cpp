// CXSpeedBuffItem.cpp

#include "CXSpeedBuffItem.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ACXSpeedBuffItem::ACXSpeedBuffItem()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	SetNetUpdateFrequency(1.0f);
	SetMinNetUpdateFrequency(1.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(SceneRoot);
	OverlapSphere->SetSphereRadius(100.0f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	OverlapSphere->SetGenerateOverlapEvents(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(SceneRoot);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACXSpeedBuffItem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this,	&ThisClass::OnOverlapSphereBeginOverlap);
	}
}

void ACXSpeedBuffItem::OnOverlapSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (HasAuthority() == false || bCollected)
	{
		return;
	}

	ACXPawn* VehiclePawn = Cast<ACXPawn>(OtherActor);
	if (IsValid(VehiclePawn) == false)
	{
		return;
	}

	ACXPlayerState* PlayerState = VehiclePawn->GetPlayerState<ACXPlayerState>();
	if (IsValid(PlayerState) == false)
	{
		return;
	}

	bCollected = true;

	PlayerState->AddBonusMaxSpeed(BonusMaxSpeedAmount);

	UE_LOG(
		LogTemplateVehicle,
		Log,
		TEXT("%s collected %s and received %.1f km/h bonus max speed"),
		*GetNameSafe(VehiclePawn),
		*GetNameSafe(this),
		BonusMaxSpeedAmount
	);

	MulticastRPCPlayPickupEffects(GetActorLocation());

	Destroy();
}

void ACXSpeedBuffItem::MulticastRPCPlayPickupEffects_Implementation(
	FVector EffectLocation
)
{
	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	if (IsValid(PickupParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			PickupParticle,
			EffectLocation,
			FRotator::ZeroRotator,
			FVector::OneVector,
			true
		);
	}

	if (IsValid(PickupSound))
	{
		UGameplayStatics::PlaySoundAtLocation(World, PickupSound, EffectLocation);
	}
}