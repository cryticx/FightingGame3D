// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingGame3DGameMode.h"
#include "FightingGame3DCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFightingGame3DGameMode::AFightingGame3DGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
