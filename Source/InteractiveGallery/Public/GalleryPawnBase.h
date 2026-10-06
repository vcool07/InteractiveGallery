#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "CameraTransitionComponent.h"
#include "InputActionValue.h"
#include "GalleryPawnBase.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class INTERACTIVEGALLERY_API AGalleryPawnBase : public APawn
{
    GENERATED_BODY()

public:
    AGalleryPawnBase();

    virtual void Tick(float DeltaTime) override;

protected:
    // Cursor + input mode live here, not in BeginPlay: BeginPlay runs at spawn, before the pawn has a controller
    virtual void PossessedBy(AController* NewController) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraTransitionComponent> CameraTransition;

    // --- Input tuning ---

    // Degrees per unit of mouse movement while dragging
    UPROPERTY(EditDefaultsOnly, Category = "Input|Orbit", meta = (ClampMin = "0"))
    float MouseOrbitSensitivity = 0.3f;

    // Degrees per second while an orbit key / stick is held
    UPROPERTY(EditDefaultsOnly, Category = "Input|Orbit", meta = (ClampMin = "0"))
    float KeyOrbitSpeed = 90.f;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Orbit")
    bool bInvertOrbit = false;

    // Mouse orbits only while a mouse button is held, so moving the cursor to the UI doesn't spin the view
    UPROPERTY(EditDefaultsOnly, Category = "Input|Orbit")
    bool bRequireMouseButtonToOrbit = true;

    // Fraction of the current distance per wheel notch (proportional steps feel the same near and far)
    UPROPERTY(EditDefaultsOnly, Category = "Input|Zoom", meta = (ClampMin = "0", ClampMax = "0.9"))
    float WheelZoomStep = 0.12f;

    // Fraction of the current distance per second while a zoom key is held
    UPROPERTY(EditDefaultsOnly, Category = "Input|Zoom", meta = (ClampMin = "0"))
    float KeyZoomSpeed = 1.0f;

    // How quickly the camera catches up with the zoom target (0 = instant)
    UPROPERTY(EditDefaultsOnly, Category = "Input|Zoom", meta = (ClampMin = "0"))
    float ZoomSmoothing = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Zoom", meta = (ClampMin = "10"))
    float MinArmLength = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Zoom", meta = (ClampMin = "10"))
    float MaxArmLength = 2000.f;

    // Remember which keyboard/gamepad keys drive each action. Mouse and wheel input arrives as per-event
    // deltas, but held keys fire every frame and must be scaled by DeltaTime; this lets us tell them apart.
    void CacheHeldKeys(const UInputMappingContext* Context);

    // Degrees to orbit for this input event
    float GetOrbitDelta(const FInputActionValue& Value, const UInputAction* Action) const;

    // Positive axis zooms in
    void ApplyZoomInput(const FInputActionValue& Value, const UInputAction* Action);

    void SetArmLengthLimits(float Min, float Max);
    void SetDesiredArmLength(float Length, bool bInstant);

public:
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void StartCameraTransitionTo(AActor* TargetActor, float Duration = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void StartCameraTransitionToTransform(const FTransform& Target, float Duration = 1.0f);

private:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> InvisibleMesh;

    float DesiredArmLength = 1000.f;

    TMap<const UInputAction*, TArray<FKey>> HeldKeysByAction;

    bool IsHeldKeyDown(const UInputAction* Action) const;
};
