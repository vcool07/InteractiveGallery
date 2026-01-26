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
    virtual void BeginPlay() override;

    // NOTE: This is named 'SpringArm', NOT 'SpringArmComponent'
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpringArmComponent* SpringArm;  // <-- CORRECT NAME

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCameraComponent* Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCameraTransitionComponent* CameraTransition;

public:
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void StartCameraTransitionTo(AActor* TargetActor, float Duration = 1.0f);

private:
    UPROPERTY()
    UStaticMeshComponent* InvisibleMesh;
};