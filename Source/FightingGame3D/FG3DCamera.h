#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FG3DCamera.generated.h"

UCLASS()
class FIGHTINGGAME3D_API AFG3DCamera : public AActor
{
	GENERATED_BODY()
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	class AActor *Player0, *Player1;
	
public:
	AFG3DCamera();
	virtual void Tick(float DeltaTime) override;
};
