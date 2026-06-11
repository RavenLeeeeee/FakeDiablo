// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MyGame.h"
#include "UObject/ConstructorHelpers.h"

AARPGPlayerCharacter::AARPGPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxAcceleration = 4096.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 4096.f;
	GetCharacterMovement()->GroundFriction = 10.f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (CharacterMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(CharacterMesh.Object);
		UE_LOG(LogMyGame, Log, TEXT("AARPGPlayerCharacter mesh assigned: %s"), *CharacterMesh.Object->GetPathName());
	}
	else
	{
		UE_LOG(LogMyGame, Warning, TEXT("AARPGPlayerCharacter mesh not found; capsule and camera are still configured"));
	}

	GetMesh()->SetHiddenInGame(false);
	GetMesh()->SetVisibility(true);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -89.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 900.f;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
}

void AARPGPlayerCharacter::MoveInWorldDirection(const FVector2D& MovementInput)
{
	const FVector Direction(MovementInput.X, MovementInput.Y, 0.f);

	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction.GetSafeNormal(), 1.f);
	}
}

void AARPGPlayerCharacter::FaceWorldPoint(const FVector& WorldPoint)
{
	const FVector ToTarget = WorldPoint - GetActorLocation();
	const FVector FlatDirection(ToTarget.X, ToTarget.Y, 0.f);

	if (!FlatDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, FlatDirection.Rotation().Yaw, 0.f));
	}
}

void AARPGPlayerCharacter::BasicAttack()
{
	UE_LOG(LogMyGame, Log, TEXT("AARPGPlayerCharacter BasicAttack"));
}

void AARPGPlayerCharacter::Dodge()
{
	UE_LOG(LogMyGame, Log, TEXT("AARPGPlayerCharacter Dodge"));
}
