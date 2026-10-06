#pragma once

#include "CoreMinimal.h"
#include "GalleryPawnBase.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "TopViewPawn.generated.h"

UCLASS()
class INTERACTIVEGALLERY_API ATopViewPawn : public AGalleryPawnBase
{
    GENERATED_BODY()

public:
    ATopViewPawn();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    // Enhanced Input Assets for TopView
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputMappingContext> TopViewMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> TopViewRotateAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> TopViewZoomAction;

    // Switch mode action (same as Walk mode)
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> SwitchAction;

    // Input functions
    void RotateCamera(const FInputActionValue& Value);
    void ZoomCamera(const FInputActionValue& Value);
    void SwitchModePressed();
};
