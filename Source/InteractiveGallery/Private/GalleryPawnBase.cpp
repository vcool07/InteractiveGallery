#include "GalleryPawnBase.h"

AGalleryPawnBase::AGalleryPawnBase()
{
    PrimaryActorTick.bCanEverTick = false;

    // Root
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Invisible mesh (prevents warning)
    InvisibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InvisibleMesh"));
    InvisibleMesh->SetupAttachment(RootComponent);
    InvisibleMesh->SetVisibility(false);
    InvisibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Spring arm
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 1000.f;
    SpringArm->bDoCollisionTest = false;
    SpringArm->bUsePawnControlRotation = false;

    // Camera
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    // Camera transition
    CameraTransition = CreateDefaultSubobject<UCameraTransitionComponent>(TEXT("CameraTransition"));

    // No collision
    SetActorEnableCollision(false);
}

void AGalleryPawnBase::BeginPlay()
{
    Super::BeginPlay();
}

void AGalleryPawnBase::StartCameraTransitionTo(AActor* TargetActor, float Duration)
{
    if (CameraTransition && TargetActor)
    {
        CameraTransition->StartTransitionToActor(TargetActor, Duration);
    }
}