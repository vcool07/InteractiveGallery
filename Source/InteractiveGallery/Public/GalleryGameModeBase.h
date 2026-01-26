
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WalkCharacter.h" 
#include "GalleryGameModeBase.generated.h"

class AGalleryPawnBase;

UENUM(BlueprintType)
enum class EGalleryMode : uint8
{
    Walk     UMETA(DisplayName = "Walk Mode"),
    TopView  UMETA(DisplayName = "Top View Mode"),
    POI      UMETA(DisplayName = "POI Mode")
};

UCLASS()
class INTERACTIVEGALLERY_API AGalleryGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGalleryModeSwitchedSignature, EGalleryMode, NewMode);

    AGalleryGameModeBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gallery")
    EGalleryMode CurrentMode;

    UPROPERTY(BlueprintAssignable, Category = "Gallery")
    FOnGalleryModeSwitchedSignature OnModeSwitched;

    UFUNCTION(BlueprintCallable, Category = "Gallery")
    void SwitchMode(EGalleryMode NewMode);

    UFUNCTION(Exec) void SwitchToWalk();
    UFUNCTION(Exec) void SwitchToTopView();
    UFUNCTION(Exec) void SwitchToPOI();

    UFUNCTION(Exec)
    void TestTransition();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    APawn* CurrentPawn;

    UPROPERTY(EditDefaultsOnly, Category = "Pawn Classes")
    TSubclassOf<ACharacter> WalkPawnClass;

    UPROPERTY(EditDefaultsOnly, Category = "Pawn Classes")
    TSubclassOf<AGalleryPawnBase> TopViewPawnClass;

    UPROPERTY(EditDefaultsOnly, Category = "Pawn Classes")
    TSubclassOf<AGalleryPawnBase> POIPawnClass;

    void ChangePawnForMode(EGalleryMode Mode);
};