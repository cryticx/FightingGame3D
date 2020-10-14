// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingGame3DCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AFightingGame3DCharacter

AFightingGame3DCharacter::AFightingGame3DCharacter() {
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	
	equip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Equip"));
	equip->SetupAttachment(GetMesh(), FName(TEXT("thumb_03_r")));

	equip->OnComponentBeginOverlap.AddDynamic(this, &AFightingGame3DCharacter::WeaponOverlapBegin);
	equip->OnComponentEndOverlap.AddDynamic(this, &AFightingGame3DCharacter::WeaponOverlapEnd);

    health = maxHealth = 100.f;
    cooldown = 0.f;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFightingGame3DCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) {
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFightingGame3DCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFightingGame3DCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFightingGame3DCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFightingGame3DCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AFightingGame3DCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AFightingGame3DCharacter::TouchStopped);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AFightingGame3DCharacter::Dash);
	PlayerInputComponent->BindAction("Dodge", IE_Pressed, this, &AFightingGame3DCharacter::Dodge);
	PlayerInputComponent->BindAction("Offensive Special", IE_Pressed, this, &AFightingGame3DCharacter::Offensive_Special);
	PlayerInputComponent->BindAction("Defensive Special", IE_Pressed, this, &AFightingGame3DCharacter::Defensive_Special);
	PlayerInputComponent->BindAction("Attack 1", IE_Pressed, this, &AFightingGame3DCharacter::Attack1);
	PlayerInputComponent->BindAction("Attack 2", IE_Pressed, this, &AFightingGame3DCharacter::Attack2);
	PlayerInputComponent->BindAction("Attack 3", IE_Pressed, this, &AFightingGame3DCharacter::Attack3);
	PlayerInputComponent->BindAction("Attack 4", IE_Pressed, this, &AFightingGame3DCharacter::Attack4);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AFightingGame3DCharacter::OnResetVR);
}


void AFightingGame3DCharacter::OnResetVR() {
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AFightingGame3DCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location) {
		Jump();
}

void AFightingGame3DCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location) {
		StopJumping();
}

void AFightingGame3DCharacter::TurnAtRate(float Rate) {
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFightingGame3DCharacter::LookUpAtRate(float Rate) {
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AFightingGame3DCharacter::MoveForward(float Value) {
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AFightingGame3DCharacter::MoveRight(float Value) {
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

float AFightingGame3DCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const & DamageEvent, class AController * EventInstigator, AActor * DamageCauser) {
	health -= DamageAmount;
	if (health <= 0.f)
		SetLifeSpan(0.01f);
	return DamageAmount;
}

void AFightingGame3DCharacter::Dash() {
	UE_LOG(LogTemp, Warning, TEXT("Dash"));
}

void AFightingGame3DCharacter::Dodge() {
	UE_LOG(LogTemp, Warning, TEXT("Dodge"));
}

void AFightingGame3DCharacter::Offensive_Special() {
	UE_LOG(LogTemp, Warning, TEXT("Offensive Special"));
}

void AFightingGame3DCharacter::Defensive_Special() {
	UE_LOG(LogTemp, Warning, TEXT("Defensive Special"));
}

void AFightingGame3DCharacter::Attack1() {
	UE_LOG(LogTemp, Warning, TEXT("Attack 1"));
}

void AFightingGame3DCharacter::Attack2() {
	UE_LOG(LogTemp, Warning, TEXT("Attack 2"));
}

void AFightingGame3DCharacter::Attack3() {
	UE_LOG(LogTemp, Warning, TEXT("Attack 3"));
}

void AFightingGame3DCharacter::Attack4() {
	UE_LOG(LogTemp, Warning, TEXT("Attack 4"));
}

void AFightingGame3DCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFightingGame3DCharacter::WeaponOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hitting"));
	}
}

void AFightingGame3DCharacter::WeaponOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not Hitting"));
	}
}