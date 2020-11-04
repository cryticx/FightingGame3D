#include "FightingGame3DGameMode.h"
#include "FightingGame3DCharacter.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AFightingGame3DGameMode::AFightingGame3DGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/GreystonePlayerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
		DefaultPawnClass = PlayerPawnBPClass.Class;
}

void AFightingGame3DGameMode::BeginPlay() {
	
	UGameplayStatics::CreatePlayer(GetWorld(), 0, true);
	UGameplayStatics::CreatePlayer(GetWorld(), 1, true);
}
