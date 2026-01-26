#include "CameraTransitionComponent.h"

UCameraTransitionComponent::UCameraTransitionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCameraTransitionComponent::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Verbose, TEXT("[CameraTransition] Component ready"));
}

void UCameraTransitionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsTransitioning)
    {
        UpdateTransition(DeltaTime);
    }
}

void UCameraTransitionComponent::StartTransitionToActor(AActor* TargetActor, float Duration)
{
    if (!TargetActor || Duration <= 0.f)
    {
        UE_LOG(LogTemp, Error, TEXT("[CameraTransition] Invalid target or duration"));
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner) return;

    StartTransform = Owner->GetTransform();
    TargetTransform = TargetActor->GetTransform();
    TransitionDuration = Duration;
    TransitionElapsed = 0.f;
    bIsTransitioning = true;

    UE_LOG(LogTemp, Display, TEXT("[CameraTransition] Starting transition to %s"),
        *TargetActor->GetName());
}

void UCameraTransitionComponent::UpdateTransition(float DeltaTime)
{
    TransitionElapsed += DeltaTime;
    float Alpha = FMath::Clamp(TransitionElapsed / TransitionDuration, 0.f, 1.f);

    // Cubic ease-in-out
    Alpha = Alpha < 0.5f ? 4.f * Alpha * Alpha * Alpha : 1.f - FMath::Pow(-2.f * Alpha + 2.f, 3.f) / 2.f;

    FVector NewLocation = FMath::Lerp(StartTransform.GetLocation(), TargetTransform.GetLocation(), Alpha);
    FRotator NewRotation = FMath::Lerp(StartTransform.GetRotation().Rotator(), TargetTransform.GetRotation().Rotator(), Alpha);

    GetOwner()->SetActorLocationAndRotation(NewLocation, NewRotation);

    if (Alpha >= 1.0f)
    {
        bIsTransitioning = false;
        UE_LOG(LogTemp, Display, TEXT("[CameraTransition] Transition complete"));
    }
}