// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FightingGame3DCharacter.generated.h"

UCLASS(config=Game)
class AFightingGame3DCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Capsule, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* sword;
	
	class AActor *Opponent;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Status, meta = (AllowPrivateAccess = "true"))
	float maxHealth;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Status, meta = (AllowPrivateAccess = "true"))
	float health;
	
    float active, cooldown, animTimer;
	
protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
public:
	AFightingGame3DCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	void Dash();
	void Dodge();
	void Offensive_Special();
	void Defensive_Special();
	void Attack1();
	void Attack2();
	void Attack3();
	void Attack4();
	
	UFUNCTION()
	void WeaponOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const & DamageEvent, class AController * EventInstigator, AActor * DamageCauser) override;
	virtual void Tick(float DeltaTime) override;
};

