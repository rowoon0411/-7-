// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXPlayerState.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	bReplicates = true;
}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACXPawn::InputAttack);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

float ACXPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HasAuthority() == false || ActualDamage <= 0.0f)
	{
		return ActualDamage;
	}

	ACXPawn* AttackerVehicle = Cast<ACXPawn>(DamageCauser);
	if (IsValid(AttackerVehicle) == false || AttackerVehicle == this)
	{
		return ActualDamage;
	}

	ACXPlayerState* VictimPlayerState = GetPlayerState<ACXPlayerState>();
	ACXPlayerState* AttackerPlayerState = AttackerVehicle->GetPlayerState<ACXPlayerState>();

	if (IsValid(VictimPlayerState) == false ||
		IsValid(AttackerPlayerState) == false)
	{
		return ActualDamage;
	}

	int32 TransferCoinAmount = 5;

	if (VictimPlayerState->TryRemoveCoins(TransferCoinAmount))
	{
		AttackerPlayerState->AddCoins(TransferCoinAmount);

		UE_LOG(
			LogTemplateVehicle,
			Log,
			TEXT("%s transferred %d coins to %s"),
			*GetNameSafe(VictimPlayerState),
			TransferCoinAmount,
			*GetNameSafe(AttackerPlayerState)
		);
	}
	else
	{
		UE_LOG(
			LogTemplateVehicle,
			Log,
			TEXT("%s has no coins to transfer to %s"),
			*GetNameSafe(VictimPlayerState),
			*GetNameSafe(AttackerPlayerState)
		);
	}

	return ActualDamage;
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// get the input magnitude for the throttle
	//float ThrottleValue = Value.Get<float>();

	// add the input
	//ChaosVehicleMovement->SetThrottleInput(ThrottleValue);

	float ThrottleValue = Value.Get<float>();

	const float CurrentForwardSpeedKmh = ChaosVehicleMovement->GetForwardSpeed() * 0.036f;
		// cm/s를 km/h로 바꾸기 위해 0.036을 곱해줌.

	const ACXPlayerState* CXPlayerState = GetPlayerState<ACXPlayerState>();
	float BonusMaxSpeedKmh = 0.0f;
	if (IsValid(CXPlayerState))
	{
		BonusMaxSpeedKmh = CXPlayerState->GetBonusMaxSpeed();
	}

	const float EffectiveMaxSpeedKmh = BaseMaxSpeedKmh + BonusMaxSpeedKmh;

	if (0.0f < ThrottleValue && EffectiveMaxSpeedKmh <= CurrentForwardSpeedKmh)
	{
		ThrottleValue = 0.0f;
	}

	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

void ACXPawn::InputAttack(const FInputActionValue& Value)
{
	if (HasAuthority() == true)
	{
		ServerRPCAttack_Implementation();
		return;
	}

	ServerRPCAttack();
}

void ACXPawn::ServerRPCAttack_Implementation()
{
	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	const FVector TraceStart = GetActorTransform().TransformPosition(AttackTraceStartOffset);
	const FVector TraceEnd = TraceStart + GetActorForwardVector() * AttackTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehicleAttackTrace), false);
	QueryParams.AddIgnoredActor(this);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	bool bHitVehicle = false;

	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		ACXPawn* HitVehicle = Cast<ACXPawn>(HitActor);

		if (IsValid(HitVehicle))
		{
			bHitVehicle = true;

			float AttackDamage = 1.0f;

			UGameplayStatics::ApplyDamage(HitVehicle, AttackDamage, GetController(), this, nullptr);

			UE_LOG(LogTemplateVehicle, Log, TEXT("Attack trace hit vehicle: %s"), *GetNameSafe(HitVehicle));
		}
		else
		{
			UE_LOG(LogTemplateVehicle, Log, TEXT("Attack trace hit non-vehicle actor: %s"), *GetNameSafe(HitActor));
		}
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Log, TEXT("Attack trace missed"));
	}

	const FVector DebugTraceEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
	const FVector ImpactPoint = bHit ? HitResult.ImpactPoint : FVector::ZeroVector;
	const FVector ImpactNormal = bHit ? HitResult.ImpactNormal : FVector::UpVector;

	MulticastRPCPlayAttackEffects(TraceStart, DebugTraceEnd, bHit, bHitVehicle, ImpactPoint, ImpactNormal);
}

void ACXPawn::MulticastRPCPlayAttackEffects_Implementation(FVector TraceStart, FVector TraceEnd, bool bTraceHit, bool bHitVehicle, FVector ImpactPoint, FVector ImpactNormal)
{
	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	const FColor DebugColor = bTraceHit ? FColor::Green : FColor::Red;

	DrawDebugLine(World, TraceStart, TraceEnd, DebugColor, false, 0.05f, 0, 3.0f);

	if (bHitVehicle == false || IsValid(HitParticle) == false)
	{
		return;
	}

	UGameplayStatics::SpawnEmitterAtLocation(
		World,
		HitParticle,
		ImpactPoint,
		ImpactNormal.Rotation(),
		FVector::OneVector,
		true
	);
}

#undef LOCTEXT_NAMESPACE
