#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GalleryGameModeBase.h"
#include "GalleryPlayerController.generated.h"

class UGalleryHUDWidget;
class UInputAction;
class UInputMappingContext;

// Owns the HUD and the UI keys, which work in every mode:
//   1 / 2         Walk / Top view (POIs are entered from the Top view markers)
//   Backspace     leave a POI (back to the mode it was entered from)
//   Left / Right  previous / next POI
//   Hold Alt      show the cursor in Walk mode so the HUD can be clicked
UCLASS()
class INTERACTIVEGALLERY_API AGalleryPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AGalleryPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;

    // Swap for a Widget Blueprint subclass to restyle the HUD
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UGalleryHUDWidget> HUDWidgetClass;

private:
    UPROPERTY()
    TObjectPtr<UGalleryHUDWidget> HUDWidget;

    // Created at runtime so the UI keys need no extra assets. Pawns call ClearAllMappings when possessed,
    // so this context is re-added (at a higher priority) in OnPossess.
    UPROPERTY()
    TObjectPtr<UInputMappingContext> UIMappingContext;

    UPROPERTY()
    TObjectPtr<UInputAction> WalkModeAction;

    UPROPERTY()
    TObjectPtr<UInputAction> TopViewModeAction;

    UPROPERTY()
    TObjectPtr<UInputAction> BackAction;

    UPROPERTY()
    TObjectPtr<UInputAction> NextPOIAction;

    UPROPERTY()
    TObjectPtr<UInputAction> PreviousPOIAction;

    UPROPERTY()
    TObjectPtr<UInputAction> ShowCursorAction;

    bool bWalkCursorActive = false;

    void CreateUIInput();
    void AddUIMappingContext();
    AGalleryGameModeBase* GetGalleryGameMode() const;

    void RequestMode(EGalleryMode Mode);
    void OnWalkModeKey() { RequestMode(EGalleryMode::Walk); }
    void OnTopViewModeKey() { RequestMode(EGalleryMode::TopView); }
    void OnBackKey();
    void OnNextPOIKey();
    void OnPreviousPOIKey();
    void OnShowCursorPressed();
    void OnShowCursorReleased();
};
