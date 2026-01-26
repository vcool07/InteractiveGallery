#pragma once

#include "CoreMinimal.h"
#include "GalleryPawnBase.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "POIPawn.generated.h"

UCLASS()
class INTERACTIVEGALLERY_API APOIPawn : public AGalleryPawnBase
{
    GENERATED_BODY()

public:
    APOIPawn();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    // Enhanced Input Assets for POI
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputMappingContext> POIMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> POIRotateAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> POIZoomAction;

    // Switch mode action (same as Walk mode)
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> SwitchAction;

    // Input functions
    void RotateAroundPOI(const FInputActionValue& Value);
    void ZoomPOI(const FInputActionValue& Value);
    void SwitchModePressed();

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float RotationSpeed = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float ZoomSpeed = 100.0f;
};