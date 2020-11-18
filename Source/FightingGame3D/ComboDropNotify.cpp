// Fill out your copyright notice in the Description page of Project Settings.


#include "ComboDropNotify.h"
#include "Engine.h"
#include "FightingGame3DCharacter.h"

void UComboDropNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) {
	AFightingGame3DCharacter* Player = (AFightingGame3DCharacter*)MeshComp->GetOwner();
	if (Player->inputBuffer != &AFightingGame3DCharacter::Attack1 || Player->inputBufferTimer <= 0) {
		Player->comboCounter = 0;
		GEngine->AddOnScreenDebugMessage(-1, 4.5, FColor::Orange, FString(TEXT("Combo Dropped")));
	}
}