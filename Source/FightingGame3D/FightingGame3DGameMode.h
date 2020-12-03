#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FightingGame3DGameMode.generated.h"

UCLASS(minimalapi)
class AFightingGame3DGameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

public:
	AFightingGame3DGameMode();
};



