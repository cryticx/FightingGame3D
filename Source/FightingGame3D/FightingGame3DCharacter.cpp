#include "FightingGame3DCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
//////////////////////////////////////////////////////////////////////////
// AFightingGame3DCharacter

const float inputBufferingTime = 0.4f;

AFightingGame3DCharacter::AFightingGame3DCharacter() {
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	sword = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Sword"));
	sword->SetupAttachment(GetMesh(), FName(TEXT("FX_Sword_Top")));
	
	sword->OnComponentBeginOverlap.AddDynamic(this, &AFightingGame3DCharacter::WeaponOverlapBegin);

	//Setup Vars
	health = maxHealth = 75.f;
	
	stamina = maxStamina = 25.f;
	staminaRegen = 8.f;
	
	actTimer = 0.f;
	comboCounter = 0;
	attacking = blocking = dodging = end_lag = false;

	//Setup Animations
	static ConstructorHelpers::FObjectFinder<UAnimMontage> animation1C0(TEXT("AnimMontage'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryA_Montage'"));
	Attack1AnimC0 = animation1C0.Object;
	static ConstructorHelpers::FObjectFinder<UAnimMontage> animation1C1(TEXT("AnimMontage'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryB_Montage'"));
	Attack1AnimC1 = animation1C1.Object;
	static ConstructorHelpers::FObjectFinder<UAnimMontage> animation1C2(TEXT("AnimMontage'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryC_Montage'"));
	Attack1AnimC2 = animation1C2.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation2(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_D_Med'"));
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
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation8(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Advance'"));
	DodgeFAnim = animation8.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation9(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Retreat'"));
	DodgeBAnim = animation9.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation10(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/EvadeLeft'"));
	DodgeLAnim = animation10.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> animation11(TEXT("AnimSequence'/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/EvadeRight'"));
	DodgeRAnim = animation11.Object;
}

void AFightingGame3DCharacter::BeginPlay() {
	Super::BeginPlay();
	AnimInstance = GetMesh()->GetAnimInstance();
}

void AFightingGame3DCharacter::Tick(float DeltaTime) {
	if (Opponent != NULL)
		GetController()->SetControlRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Opponent->GetActorLocation()));
	else if (((APlayerController*)GetController())->NetPlayerIndex == 0 && UGameplayStatics::GetPlayerController(GetWorld(), 1) != NULL) 
		Opponent = (AActor*)(UGameplayStatics::GetPlayerController(GetWorld(), 1)->GetCharacter());
	else if (UGameplayStatics::GetPlayerController(GetWorld(), 0) != NULL) 
		Opponent = (AActor*)(UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetCharacter());
	
	if (stamina < maxStamina) {
		if (blocking)
			stamina += 0.2f * staminaRegen * DeltaTime;
		else
			stamina += staminaRegen * DeltaTime;
		
		if (stamina > maxStamina)
			stamina = maxStamina;
	}
	
	if (actTimer <= 0.f) {
		attacking = dodging = attackHit = end_lag = blocking = false;
		
		if (inputBufferTimer > 0.f)
			(this->*inputBuffer)();
		
		inputBufferTimer = 0.f;
	}
	else {
		actTimer -= DeltaTime;
		if (inputBufferTimer > 0.f)
			inputBufferTimer -= DeltaTime;
		if (end_lag)
			AnimInstance->Montage_SetPlayRate(AnimInstance->GetCurrentActiveMontage(),.35f);
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

	PlayerInputComponent->BindAction("Dodge", IE_Pressed, this, &AFightingGame3DCharacter::Dodge);
	PlayerInputComponent->BindAction("Offensive Special", IE_Pressed, this, &AFightingGame3DCharacter::Offensive_Special);
	PlayerInputComponent->BindAction("Defensive Special", IE_Pressed, this, &AFightingGame3DCharacter::Defensive_Special);
	PlayerInputComponent->BindAction("Attack 1", IE_Pressed, this, &AFightingGame3DCharacter::Attack1);
	PlayerInputComponent->BindAction("Attack 2", IE_Pressed, this, &AFightingGame3DCharacter::Attack2);
	PlayerInputComponent->BindAction("Attack 3", IE_Pressed, this, &AFightingGame3DCharacter::Attack3);
	PlayerInputComponent->BindAction("Attack 4", IE_Pressed, this, &AFightingGame3DCharacter::Attack4);
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
		AFightingGame3DCharacter* Other_Player = (AFightingGame3DCharacter*)DamageCauser;
		LaunchCharacter(Other_Player->GetActorForwardVector() * 400.f, false, false);
		if (blocking) {
			if (!SpendStamina(20.f)) {
				actTimer = 4.0f;
				blocking = false;
				AnimInstance->PlaySlotAnimationAsDynamicMontage(HurtAnim, TEXT("DefaultSlot"), 0.05f, 0.05f, 1.f, 1);
			}
			else {
				Other_Player->end_lag = true;
				Other_Player->actTimer *= 1.5;
			}
		}
		else {
			health -= DamageAmount;
			actTimer = 0.8f;
			AnimInstance->PlaySlotAnimationAsDynamicMontage(HurtAnim, TEXT("DefaultSlot"), 0.05f, 0.05f, 1.f, 1);
		}
		if (health <= 0.f) {
			DetachFromControllerPendingDestroy();
			GetMesh()->SetCollisionProfileName(FName(TEXT("Ragdoll")), true);
			GetMesh()->SetSimulatePhysics(true);
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SetActorTickEnabled(false);
		}
		return DamageAmount;
	}
	return 0.f;
}

void AFightingGame3DCharacter::Dodge() {
	if (actTimer <= 0.f) {
		if (SpendStamina(20.f)) {
			if (back) {
				AnimInstance->PlaySlotAnimationAsDynamicMontage(DodgeBAnim, TEXT("DefaultSlot"), .1f, .1f, 1.f, 1);
				LaunchCharacter(GetActorForwardVector() * -3000.f, true, false);
				dodging = true;
				actTimer = 0.85f;
			}
			else if (left) {
				AnimInstance->PlaySlotAnimationAsDynamicMontage(DodgeLAnim, TEXT("DefaultSlot"), .1f, .1f, 1.f, 1);
				LaunchCharacter(GetActorRightVector() * -3000.f, true, false);
				dodging = true;
				actTimer = 0.6f;
			}
			else if (right) {
				AnimInstance->PlaySlotAnimationAsDynamicMontage(DodgeRAnim, TEXT("DefaultSlot"), .1f, .1f, 1.f, 1);
				LaunchCharacter(GetActorRightVector() * 3000.f, true, false);
				dodging = true;
				actTimer = 0.6f;
			}
			else { // dash forward by default
				AnimInstance->PlaySlotAnimationAsDynamicMontage(DodgeFAnim, TEXT("DefaultSlot"), .1f, .1f, 1.f, 1);
				LaunchCharacter(GetActorForwardVector() * 3000.f, true, false);
				actTimer = 0.59f;
			}
		}
	}
	else {
		inputBufferTimer = inputBufferingTime;
		inputBuffer = &AFightingGame3DCharacter::Dodge;
	}
}

void AFightingGame3DCharacter::Offensive_Special() {
	if (actTimer <= 0.f) {
		if (SpendStamina(25.f)) {
			attacking = true;
			AnimInstance->PlaySlotAnimationAsDynamicMontage(OSpecialAnim, TEXT("DefaultSlot"), 0.05f, 0.05f, 1.f, 1);
			LaunchCharacter(GetActorForwardVector() * 500.f + FVector(0.f, 0.f, 200.f), true, false);
			actTimer = 1.87f;
			attackDamage = 12.f;
		}
	}
	else {
		inputBufferTimer = inputBufferingTime;
		inputBuffer = &AFightingGame3DCharacter::Offensive_Special;
	}
}

void AFightingGame3DCharacter::Defensive_Special() {
	if (actTimer <= 0.f) {
		blocking = true;
		actTimer = 0.94f;
		AnimInstance->PlaySlotAnimationAsDynamicMontage(DSpecialAnim, TEXT("DefaultSlot"), 0.05f, 0.05f, 1.f, 1);
	}
	else {
		inputBufferTimer = inputBufferingTime;
		inputBuffer = &AFightingGame3DCharacter::Defensive_Special;
	}
}

void AFightingGame3DCharacter::Attack1() {
	if (actTimer <= 0.f) {
		if (SpendStamina(15.f)) {
			attacking = true;
			attackDamage = 8.f;
			if(comboCounter == 0) {
				AnimInstance->Montage_Play(Attack1AnimC0, 1.0f, EMontagePlayReturnType::Duration, 0.0f, true);
				actTimer = 1.67f;
			}
			if (comboCounter == 1) {
				AnimInstance->Montage_Play(Attack1AnimC1, 1.0f, EMontagePlayReturnType::Duration, 0.0f, true);
				actTimer = 1.4f;
			}
			if (comboCounter == 2) {
				AnimInstance->Montage_Play(Attack1AnimC2, 1.0f, EMontagePlayReturnType::Duration, 0.0f, true);
				actTimer = 1.87f;
			}
		}
	}
	else {
		inputBufferTimer = 1.2f;
		inputBuffer = &AFightingGame3DCharacter::Attack1;
	}
}

void AFightingGame3DCharacter::Attack2() {
	if (actTimer <= 0.f && SpendStamina(10.f)) {
		attacking = true;
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Attack2Anim, TEXT("DefaultSlot"), 0.05f, 0.05f, 1.f, 1);
		actTimer = 0.8f;
		attackDamage = 8.f;
	}
	else {
		inputBufferTimer = inputBufferingTime;
		inputBuffer = &AFightingGame3DCharacter::Attack2;
	}
}

void AFightingGame3DCharacter::Attack3() {
	if (actTimer <= 0.f && SpendStamina(10.f)) {
		attacking = true;
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Attack3Anim, TEXT("DefaultSlot"), 0.05f, 0.05f, 1.f, 1);
		actTimer = 1.27f;
		attackDamage = 8.f;
	}
	else {
		inputBufferTimer = inputBufferingTime;
		inputBuffer = &AFightingGame3DCharacter::Attack3;
	}
}

void AFightingGame3DCharacter::Attack4() {
	if (actTimer <= 0.f && SpendStamina(15.f)) {
		attacking = true;
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Attack4Anim, TEXT("DefaultSlot"), 0.05f, 0.05f, 1.f, 1);
		actTimer = 1.87f;
		attackDamage = 12.f;
	}
	else {
		inputBufferTimer = inputBufferingTime;
		inputBuffer = &AFightingGame3DCharacter::Attack4;
	}
}

void AFightingGame3DCharacter::WeaponOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (attacking && !attackHit && OtherActor && (OtherActor != this) && OtherComp) {
		FDamageEvent damage = FDamageEvent();
		OtherActor->TakeDamage(attackDamage, damage, Controller, this);
		attackHit = true;
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

void AFightingGame3DCharacter::ClearBuffer() {}
