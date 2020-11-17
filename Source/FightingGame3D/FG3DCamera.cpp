#include "FG3DCamera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

AFG3DCamera::AFG3DCamera()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
}

void AFG3DCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Player0 != NULL) {
		CameraBoom->TargetArmLength = Player0->GetDistanceTo(Player1) / 2;
		SetActorLocation(((Player0->GetActorLocation() + Player1->GetActorLocation()) / 2) + FVector(-220.f, 0.f, 440.f));
	}
	else if (UGameplayStatics::GetPlayerController(GetWorld(), 0) != NULL && UGameplayStatics::GetPlayerController(GetWorld(), 1) != NULL) {
		Player0 = (AActor*) UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetCharacter();
		Player1 = (AActor*) UGameplayStatics::GetPlayerController(GetWorld(), 1)->GetCharacter();
		
		UGameplayStatics::GetPlayerController(GetWorld(), 0)->bAutoManageActiveCameraTarget = false;
		UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetViewTarget(this);
		UGameplayStatics::GetPlayerController(GetWorld(), 1)->bAutoManageActiveCameraTarget = false;
		UGameplayStatics::GetPlayerController(GetWorld(), 1)->SetViewTarget(this);
	}
}

