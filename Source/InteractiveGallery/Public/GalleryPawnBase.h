#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "CameraTransitionComponent.h"
#include "GalleryPawnBase.generated.h"

UCLASS()
class INTERACTIVEGALLERY_API AGalleryPawnBase : public APawn
{
    GENERATED_BODY()

public:
    AGalleryPawnBase();

protected:
    // Cursor + input mode live here, not in BeginPlay: BeginPlay runs at spawn, before the pawn has a controller
    virtual void PossessedBy(AController* NewController) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraTransitionComponent> CameraTransition;

public:
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void StartCameraTransitionTo(AActor* TargetActor, float Duration = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void StartCameraTransitionToTransform(const FTransform& Target, float Duration = 1.0f);

private:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> InvisibleMesh;
};
