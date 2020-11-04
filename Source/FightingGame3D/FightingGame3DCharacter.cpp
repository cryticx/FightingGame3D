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
	staminaRegen = 20.f;
	
	actTimer = 0.f;
	attacking = blocking = dodging = false;

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
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation8(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/F_Dodge'"));
	DodgeFAnim = animation8.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation9(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Backward_Dodge'"));
	DodgeBAnim = animation9.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation10(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Left_Dodge'"));
	DodgeLAnim = animation10.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation11(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Right_Dodge'"));
	DodgeRAnim = animation11.Object;
}

void AFightingGame3DCharacter::Tick(float DeltaTime) {
	if (Opponent != NULL)
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Opponent->GetActorLocation()), ETeleportType::None);
	else if (((APlayerController*) GetController())->NetPlayerIndex == 0 && UGameplayStatics::GetPlayerController(GetWorld(), 1) != NULL)
		Opponent = (AActor*) (UGameplayStatics::GetPlayerController(GetWorld(), 1)->GetCharacter());
	else if (UGameplayStatics::GetPlayerController(GetWorld(), 0) != NULL)
		Opponent = (AActor*) (UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetCharacter());
	
	if (stamina < maxStamina) {
		stamina += staminaRegen * DeltaTime;
		if (stamina > maxStamina)
			stamina = maxStamina;
	}
	
	if (actTimer <= 0.f)
		attacking = dodging = attackHit = false;
	else
		actTimer -= DeltaTime;
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
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
	if (Value < 0)
		back = true;
	else
		back = false;
}

void AFightingGame3DCharacter::MoveRight(float Value) {
	if ( (Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
	if (Value < 0) {
		left = true;
		right = false;
	}
	else {
		left = false;
		right = true;
	}
}

float AFightingGame3DCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) {
	if (!dodging)
	{
		LaunchCharacter(((AFightingGame3DCharacter*)DamageCauser)->GetActorForwardVector() * 400.f, false, false);
		GetMesh()->PlayAnimation(HurtAnim, false);
		health -= DamageAmount;
		actTimer = 0.5f;
		if (health <= 0.f)
			SetLifeSpan(0.01f);
		return DamageAmount;
	}
	return 0.f;
}

void AFightingGame3DCharacter::Dodge() {
	if (actTimer <= 0.f && SpendStamina(20.f)) {
		if (back) {
			GetMesh()->PlayAnimation(DodgeBAnim, false);
			dodge_launch = -GetActorForwardVector();
			dodging = true;
			actTimer = 0.776f;
		}
		else if (left) {
			GetMesh()->PlayAnimation(DodgeLAnim, false);
			dodge_launch = -GetActorRightVector();
			dodging = true;
			actTimer = 1.154f;
		}
		else if (right) {
			GetMesh()->PlayAnimation(DodgeRAnim, false);
			dodge_launch = GetActorRightVector();
			dodging = true;
			actTimer = 1.184f;
		}
		else { // dash forward by default
			GetMesh()->PlayAnimation(DodgeFAnim, false);
			dodge_launch = GetActorForwardVector();
			actTimer = 0.824f;
		}
		dodge_launch *= 1200;
	}
}

void AFightingGame3DCharacter::Offensive_Special() {
	if (actTimer <= 0.f && SpendStamina(25.f)) {
		attacking = true;
		GetMesh()->PlayAnimation(OSpecialAnim, false);
		actTimer = 1.867f;
		attackDamage = 12.f;
	}
}

void AFightingGame3DCharacter::Defensive_Special() {
	blocking = true;
	GetMesh()->PlayAnimation(DSpecialAnim, false);
}

void AFightingGame3DCharacter::Attack1() {
	if (actTimer <= 0.f && SpendStamina(15.f)) {
		attacking = true;
		GetMesh()->PlayAnimation(Attack1Anim, false);
		actTimer = 1.4f;
		attackDamage = 8.f;
	}
}

void AFightingGame3DCharacter::Attack2() {
	if (actTimer <= 0.f && SpendStamina(10.f)) {
		attacking = true;
		GetMesh()->PlayAnimation(Attack2Anim, false);
		actTimer = 0.8f;
		attackDamage = 8.f;
	}
}

void AFightingGame3DCharacter::Attack3() {
	if (actTimer <= 0.f && SpendStamina(10.f)) {
		attacking = true;
		GetMesh()->PlayAnimation(Attack3Anim, false);
		actTimer = 1.267f;
		attackDamage = 8.f;
	}
}

void AFightingGame3DCharacter::Attack4() {
	if (actTimer <= 0.f && SpendStamina(15.f)) {
		attacking = true;
		GetMesh()->PlayAnimation(Attack4Anim, false);
		actTimer = 1.867f;
		attackDamage = 12.f;
	}
}

void AFightingGame3DCharacter::WeaponOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (attacking && !attackHit && OtherActor && (OtherActor != this) && OtherComp)
	{
		FDamageEvent damage = FDamageEvent();
		OtherActor->TakeDamage(attackDamage, damage, Controller, this);
	}
}

bool AFightingGame3DCharacter::SpendStamina(float amount) {
	if (stamina >= amount) {
		stamina -= amount;
		return true;
	}
	else
		return false;
}
