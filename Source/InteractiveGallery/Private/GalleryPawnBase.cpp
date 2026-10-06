#include "GalleryPawnBase.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AGalleryPawnBase::AGalleryPawnBase()
{
    // Ticks only to smooth zoom
    PrimaryActorTick.bCanEverTick = true;

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

void AGalleryPawnBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    DesiredArmLength = FMath::Clamp(SpringArm->TargetArmLength, MinArmLength, MaxArmLength);

    if (APlayerController* PC = Cast<APlayerController>(NewController))
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }
}

void AGalleryPawnBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!FMath::IsNearlyEqual(SpringArm->TargetArmLength, DesiredArmLength, 0.1f))
    {
        SpringArm->TargetArmLength = ZoomSmoothing > 0.f
            ? FMath::FInterpTo(SpringArm->TargetArmLength, DesiredArmLength, DeltaTime, ZoomSmoothing)
            : DesiredArmLength;
    }
}

void AGalleryPawnBase::CacheHeldKeys(const UInputMappingContext* Context)
{
    HeldKeysByAction.Reset();
    if (!Context)
    {
        return;
    }

    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        // Every mouse key (axes, buttons, wheel) reports per-event; everything else is a held key or stick
        const FKey& Key = Mapping.Key;
        const bool bIsMouseKey = Key.IsMouseButton() || Key == EKeys::MouseX || Key == EKeys::MouseY
            || Key == EKeys::MouseWheelAxis || Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown;

        if (Mapping.Action && !bIsMouseKey)
        {
            HeldKeysByAction.FindOrAdd(Mapping.Action).AddUnique(Mapping.Key);
        }
    }
}

bool AGalleryPawnBase::IsHeldKeyDown(const UInputAction* Action) const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    const TArray<FKey>* Keys = HeldKeysByAction.Find(Action);
    if (!PC || !Keys)
    {
        return false;
    }

    for (const FKey& Key : *Keys)
    {
        if (Key.IsAxis1D() ? FMath::Abs(PC->GetInputAnalogKeyState(Key)) > 0.f : PC->IsInputKeyDown(Key))
        {
            return true;
        }
    }
    return false;
}

float AGalleryPawnBase::GetOrbitDelta(const FInputActionValue& Value, const UInputAction* Action) const
{
    const float Axis = Value.Get<float>() * (bInvertOrbit ? -1.f : 1.f);

    if (IsHeldKeyDown(Action))
    {
        return Axis * KeyOrbitSpeed * GetWorld()->GetDeltaSeconds();
    }

    if (bRequireMouseButtonToOrbit)
    {
        const APlayerController* PC = Cast<APlayerController>(GetController());
        if (!PC || !(PC->IsInputKeyDown(EKeys::LeftMouseButton) || PC->IsInputKeyDown(EKeys::RightMouseButton)))
        {
            return 0.f;
        }
    }

    return Axis * MouseOrbitSensitivity;
}

void AGalleryPawnBase::ApplyZoomInput(const FInputActionValue& Value, const UInputAction* Action)
{
    const float Axis = Value.Get<float>();
    const float Fraction = IsHeldKeyDown(Action)
        ? Axis * KeyZoomSpeed * GetWorld()->GetDeltaSeconds()
        : Axis * WheelZoomStep;

    SetDesiredArmLength(DesiredArmLength * (1.f - FMath::Clamp(Fraction, -0.9f, 0.9f)), false);
}

void AGalleryPawnBase::SetArmLengthLimits(float Min, float Max)
{
    MinArmLength = FMath::Max(10.f, FMath::Min(Min, Max));
    MaxArmLength = FMath::Max(Min, Max);
    SetDesiredArmLength(DesiredArmLength, false);
}

void AGalleryPawnBase::SetDesiredArmLength(float Length, bool bInstant)
{
    DesiredArmLength = FMath::Clamp(Length, MinArmLength, MaxArmLength);
    if (bInstant)
    {
        SpringArm->TargetArmLength = DesiredArmLength;
    }
}

void AGalleryPawnBase::StartCameraTransitionTo(AActor* TargetActor, float Duration)
{
    if (CameraTransition && TargetActor)
    {
        CameraTransition->StartTransitionToActor(TargetActor, Duration);
    }
}

void AGalleryPawnBase::StartCameraTransitionToTransform(const FTransform& Target, float Duration)
{
    if (CameraTransition)
    {
        CameraTransition->StartTransitionToTransform(Target, Duration);
    }
}
