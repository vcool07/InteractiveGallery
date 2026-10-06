#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GalleryGameModeBase.generated.h"

class ACharacter;
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

    // Walk -> TopView -> POI -> Walk
    UFUNCTION(BlueprintCallable, Category = "Gallery")
    void CycleMode();

    UFUNCTION(Exec) void SwitchToWalk();
    UFUNCTION(Exec) void SwitchToTopView();
    UFUNCTION(Exec) void SwitchToPOI();

    UFUNCTION(Exec)
    void TestTransition();

protected:
    virtual void BeginPlay() override;

    // The engine spawns the first pawn through these, so it is always the pawn for CurrentMode
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

private:
    UPROPERTY()
    TObjectPtr<APawn> CurrentPawn;

    UPROPERTY(EditDefaultsOnly, Category = "Pawn Classes")
    TSubclassOf<ACharacter> WalkPawnClass;

    UPROPERTY(EditDefaultsOnly, Category = "Pawn Classes")
    TSubclassOf<AGalleryPawnBase> TopViewPawnClass;

    UPROPERTY(EditDefaultsOnly, Category = "Pawn Classes")
    TSubclassOf<AGalleryPawnBase> POIPawnClass;

    // Camera blend time when switching modes (0 = hard cut)
    UPROPERTY(EditDefaultsOnly, Category = "Gallery")
    float ModeBlendTime = 0.75f;

    // Where the walk pawn was when we left Walk mode, so returning puts you back there
    FTransform LastWalkTransform;
    bool bHasLastWalkTransform = false;

    UClass* GetPawnClassForMode(EGalleryMode Mode) const;
    FTransform GetSpawnTransformForMode(EGalleryMode Mode) const;
    APawn* SpawnPawnForMode(EGalleryMode Mode, AController* ForController);
    void ChangePawnForMode(EGalleryMode Mode);
};
