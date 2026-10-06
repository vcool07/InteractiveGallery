#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "WalkCharacter.generated.h"

UCLASS()
class INTERACTIVEGALLERY_API AWalkCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AWalkCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<class UCameraComponent> FirstPersonCamera;

    // Enhanced Input
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputMappingContext> InputMapping;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TSoftObjectPtr<UInputAction> SwitchAction;

    // Input functions
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void JumpPressed();
    void SwitchModePressed();
};