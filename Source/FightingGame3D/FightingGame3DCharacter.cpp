#include "FightingGame3DCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AFightingGame3DCharacter

AFightingGame3DCharacter::AFightingGame3DCharacter() {
	PrimaryActorTick.bCanEverTick = true;
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

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	sword = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Sword"));
	sword->SetupAttachment(GetMesh(), FName(TEXT("FX_Sword_Top")));
	
	sword->OnComponentBeginOverlap.AddDynamic(this, &AFightingGame3DCharacter::WeaponOverlapBegin);

	//Setup Vars
    health = maxHealth = 100.0f;
	stamina = maxStamina = 100.0f;
	stamregen = 10;
	actionTimer = 100;
	invincibilityTimer = 100;
	hitstunTimer = 0;
	can_act = true;
	acting = false;

	//Setup Animations
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation1(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryB'"));
	Attack1Anim = animation1.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation2(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_A_Med'"));
	Attack2Anim = animation2.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation3(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_E'"));
	Attack3Anim = animation3.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation4(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_C_Med'"));
	Attack4Anim = animation4.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation5(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_R'"));
	OSpecialAnim = animation5.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation6(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_Q'"));
	DSpecialAnim = animation6.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation7(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/HitReact_Front'"));
	HurtAnim = animation7.Object;
}

void AFightingGame3DCharacter::Tick(float DeltaTime) {
	if (Opponent != NULL)
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Opponent->GetActorLocation()), ETeleportType::None);
	else if (((APlayerController*) GetController())->NetPlayerIndex == 0 && UGameplayStatics::GetPlayerController(GetWorld(), 1) != NULL)
		Opponent = (AActor*) (UGameplayStatics::GetPlayerController(GetWorld(), 1)->GetCharacter());
	else if (UGameplayStatics::GetPlayerController(GetWorld(), 0) != NULL)
		Opponent = (AActor*) (UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetCharacter());
	
	if (stamregen == 0) {
		RegenStamina();
		stamregen = 10;
	}
	else {
		stamregen--;
	}
	if (acting) {
		if (actionTimer == 0) {
			can_act = true;
			actionTimer = 100;
			acting = false;
		}
		else{
			can_act = false;
			actionTimer--;
			//UE_LOG(LogTemp, Warning, TEXT("Action Timer : %d"), actionTimer);
		}
	}
	if (invincibilityTimer>0) {
		UE_LOG(LogTemp, Warning, TEXT("Invincibility Timer : %d"), invincibilityTimer);
		invincibilityTimer--;
	}
	if (hitstunTimer > 0) {
		hitstunTimer--;
		can_act = false;
	}
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
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AFightingGame3DCharacter::Dash);
	PlayerInputComponent->BindAction("Dodge", IE_Pressed, this, &AFightingGame3DCharacter::Dodge);
	PlayerInputComponent->BindAction("Offensive Special", IE_Pressed, this, &AFightingGame3DCharacter::Offensive_Special);
	PlayerInputComponent->BindAction("Defensive Special", IE_Pressed, this, &AFightingGame3DCharacter::Defensive_Special);
	PlayerInputComponent->BindAction("Attack 1", IE_Pressed, this, &AFightingGame3DCharacter::Attack1);
	PlayerInputComponent->BindAction("Attack 2", IE_Pressed, this, &AFightingGame3DCharacter::Attack2);
	PlayerInputComponent->BindAction("Attack 3", IE_Pressed, this, &AFightingGame3DCharacter::Attack3);
	PlayerInputComponent->BindAction("Attack 4", IE_Pressed, this, &AFightingGame3DCharacter::Attack4);
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
	if ((Controller != NULL) && (Value != 0.0f) && can_act)
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
	if ( (Controller != NULL) && (Value != 0.0f) && can_act)
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

float AFightingGame3DCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) {
	UE_LOG(LogTemp, Warning, TEXT("Damage"));
	AFightingGame3DCharacter* other_character = (AFightingGame3DCharacter*)DamageCauser;
	if (invincibilityTimer == 0 && other_character->acting)
	{
		FVector launch = other_character->GetActorForwardVector();
		launch *= 100;
		LaunchCharacter(launch,false,false);
		GetMesh()->PlayAnimation(HurtAnim, false);
		health -= DamageAmount;
		invincibilityTimer = 100;
		hitstunTimer = 50;
		if (health <= 0.f)SetLifeSpan(0.01f);
		return DamageAmount;
	}
	return DamageAmount;
}

void AFightingGame3DCharacter::Dash() {
	if (SpendStamina()&& can_act) {
		UE_LOG(LogTemp, Warning, TEXT("Dash"));
	}
}

void AFightingGame3DCharacter::Dodge() {
	if (SpendStamina() && can_act) {
		UE_LOG(LogTemp, Warning, TEXT("Dodge"));
	}
}

void AFightingGame3DCharacter::Offensive_Special() {
	if (SpendStamina() && can_act) {
		GetMesh()->PlayAnimation(OSpecialAnim, false);
		actionTimer = 150;
		UE_LOG(LogTemp, Warning, TEXT("Offensive Special"));
	}
}

void AFightingGame3DCharacter::Defensive_Special() {
	if (SpendStamina() && can_act) {
		GetMesh()->PlayAnimation(DSpecialAnim, false);
		UE_LOG(LogTemp, Warning, TEXT("Defensive Special"));
	}
}

void AFightingGame3DCharacter::Attack1() {
	if (SpendStamina() && can_act) {
		GetMesh()->PlayAnimation(Attack1Anim, false);
		UE_LOG(LogTemp, Warning, TEXT("Attack 1"));
	}
}

void AFightingGame3DCharacter::Attack2() {
	if (SpendStamina() && can_act) {
		//for testing purposes
		//actionTimer = 30;
		GetMesh()->PlayAnimation(Attack2Anim, false);
		UE_LOG(LogTemp, Warning, TEXT("Attack 2"));
	}
}

void AFightingGame3DCharacter::Attack3() {
	if (SpendStamina() && can_act) {
		GetMesh()->PlayAnimation(Attack3Anim, false);
		UE_LOG(LogTemp, Warning, TEXT("Attack 3"));
	}
}

void AFightingGame3DCharacter::Attack4() {
	if (SpendStamina() && can_act) {
		GetMesh()->PlayAnimation(Attack4Anim, false);
		UE_LOG(LogTemp, Warning, TEXT("Attack 4"));
	}
}

void AFightingGame3DCharacter::WeaponOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		FDamageEvent damage = FDamageEvent();
		OtherActor->TakeDamage(10.0f, damage, Controller, this);
		AFightingGame3DCharacter* other_character = (AFightingGame3DCharacter*)OtherActor;
		UE_LOG(LogTemp, Warning, TEXT("Health : %f"), other_character->health);
		UE_LOG(LogTemp, Warning, TEXT("Hitting"));
	}
}

bool AFightingGame3DCharacter::SpendStamina() {
	UE_LOG(LogTemp, Warning, TEXT("Stamina : %f"), stamina);
	if (stamina >= 9.9f && !acting) {
		stamina -= 10.0f;
		acting = true;
		return true;
	}
	else {
		return false;
	}
}

void AFightingGame3DCharacter::RegenStamina() {
	if (stamina < maxStamina) {
		stamina += 0.5f;
		//UE_LOG(LogTemp, Warning, TEXT("Stamina : %f"), stamina);
		return;
	}
	else {
		return;
	}
}
