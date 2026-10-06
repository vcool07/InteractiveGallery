#pragma once

#include "CoreMinimal.h"
#include "GalleryPawnBase.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "POIPawn.generated.h"

class APOITarget;

UCLASS()
class INTERACTIVEGALLERY_API APOIPawn : public AGalleryPawnBase
{
    GENERATED_BODY()

public:
    APOIPawn();

    // Orbit around Target's pivot; bAnimate flies there instead of snapping
    UFUNCTION(BlueprintCallable, Category = "POI")
    void FocusOn(APOITarget* Target, bool bAnimate);

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

    UPROPERTY(EditDefaultsOnly, Category = "POI", meta = (ClampMin = "0.05"))
    float FocusTransitionTime = 1.2f;

    // Input functions
    void RotateAroundPOI(const FInputActionValue& Value);
    void ZoomPOI(const FInputActionValue& Value);
    void SwitchModePressed();
};
