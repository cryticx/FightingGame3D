// Fill out your copyright notice in the Description page of Project Settings.

#include "ComboNotify.h"
#include "Engine.h"
#include "FightingGame3DCharacter.h"

 void UComboNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) {
	 AFightingGame3DCharacter* Player = (AFightingGame3DCharacter*)MeshComp->GetOwner();
	 if (Player->comboCounter == 2) {
		 Player->comboCounter = 0;
	 }
	 else {
		 Player->comboCounter++;
	 }
	 GEngine->AddOnScreenDebugMessage(-1, 4.5, FColor::Orange, FString::FromInt(Player->comboCounter));
	 if (Player->inputBuffer == &AFightingGame3DCharacter::Attack1 && Player->inputBufferTimer > 0) {
		 //GEngine->AddOnScreenDebugMessage(-1, 4.5, FColor::Orange, FString(TEXT("Quick Combo")));
		 Player->inputBuffer = &AFightingGame3DCharacter::ClearBuffer;
		 Player->actTimer = 0;
		 Player->Attack1();		
	 }
}