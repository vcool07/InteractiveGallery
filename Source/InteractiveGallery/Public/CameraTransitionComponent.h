#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraTransitionComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class INTERACTIVEGALLERY_API UCameraTransitionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCameraTransitionComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void StartTransitionToActor(AActor* TargetActor, float Duration = 1.0f);

    UFUNCTION(BlueprintPure, Category = "Camera")
    bool IsTransitioning() const { return bIsTransitioning; }

private:
    bool bIsTransitioning = false;
    float TransitionElapsed = 0.f;
    float TransitionDuration = 1.f;
    FTransform StartTransform;
    FTransform TargetTransform;

    void UpdateTransition(float DeltaTime);
};