#include "CameraTransitionComponent.h"

UCameraTransitionComponent::UCameraTransitionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
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
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Error, TEXT("[CameraTransition] Invalid target actor"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[CameraTransition] Starting transition to %s"), *TargetActor->GetName());
    StartTransitionToTransform(TargetActor->GetActorTransform(), Duration);
}

void UCameraTransitionComponent::StartTransitionToTransform(const FTransform& Target, float Duration)
{
    AActor* Owner = GetOwner();
    if (!Owner || Duration <= 0.f)
    {
        UE_LOG(LogTemp, Error, TEXT("[CameraTransition] Invalid owner or duration"));
        return;
    }

    StartTransform = Owner->GetActorTransform();
    TargetTransform = Target;
    TransitionDuration = Duration;
    TransitionElapsed = 0.f;
    bIsTransitioning = true;
    SetComponentTickEnabled(true);
}

void UCameraTransitionComponent::UpdateTransition(float DeltaTime)
{
    TransitionElapsed += DeltaTime;
    float Alpha = FMath::Clamp(TransitionElapsed / TransitionDuration, 0.f, 1.f);

    // Cubic ease-in-out
    Alpha = Alpha < 0.5f ? 4.f * Alpha * Alpha * Alpha : 1.f - FMath::Pow(-2.f * Alpha + 2.f, 3.f) / 2.f;

    const FVector NewLocation = FMath::Lerp(StartTransform.GetLocation(), TargetTransform.GetLocation(), Alpha);
    // Slerp takes the shortest path (rotator lerp can spin the long way round at the +-180 seam)
    const FQuat NewRotation = FQuat::Slerp(StartTransform.GetRotation(), TargetTransform.GetRotation(), Alpha);

    GetOwner()->SetActorLocationAndRotation(NewLocation, NewRotation);

    if (Alpha >= 1.0f)
    {
        bIsTransitioning = false;
        SetComponentTickEnabled(false);
        UE_LOG(LogTemp, Display, TEXT("[CameraTransition] Transition complete"));
    }
}
