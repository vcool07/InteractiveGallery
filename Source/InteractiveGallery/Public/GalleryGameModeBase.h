#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GalleryGameModeBase.generated.h"

class ACharacter;
class AGalleryPawnBase;
class APOITarget;

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
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPOIChangedSignature, APOITarget*, NewPOI);

    AGalleryGameModeBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gallery")
    EGalleryMode CurrentMode;

    UPROPERTY(BlueprintAssignable, Category = "Gallery")
    FOnGalleryModeSwitchedSignature OnModeSwitched;

    UFUNCTION(BlueprintCallable, Category = "Gallery")
    void SwitchMode(EGalleryMode NewMode);

    // Walk <-> TopView; from POI, back to the mode it was entered from
    UFUNCTION(BlueprintCallable, Category = "Gallery")
    void CycleMode();

    UFUNCTION(Exec) void SwitchToWalk();
    UFUNCTION(Exec) void SwitchToTopView();
    UFUNCTION(Exec) void SwitchToPOI();

    UFUNCTION(Exec)
    void TestTransition();

    // --- Points of interest (every APOITarget in the level, sorted by Order) ---

    UPROPERTY(BlueprintAssignable, Category = "Gallery|POI")
    FOnPOIChangedSignature OnPOIChanged;

    // Switches to POI mode if needed, then flies to the POI
    UFUNCTION(BlueprintCallable, Category = "Gallery|POI")
    void GoToPOI(int32 Index);

    UFUNCTION(Exec, BlueprintCallable, Category = "Gallery|POI")
    void NextPOI();

    UFUNCTION(Exec, BlueprintCallable, Category = "Gallery|POI")
    void PreviousPOI();

    // Leave POI mode, back to Top view or Walk (whichever it was entered from)
    UFUNCTION(Exec, BlueprintCallable, Category = "Gallery|POI")
    void ReturnFromPOI();

    UFUNCTION(BlueprintPure, Category = "Gallery|POI")
    EGalleryMode GetModeBeforePOI() const { return ModeBeforePOI; }

    UFUNCTION(BlueprintPure, Category = "Gallery|POI")
    APOITarget* GetPOI(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Gallery|POI")
    APOITarget* GetCurrentPOI() const;

    UFUNCTION(BlueprintPure, Category = "Gallery|POI")
    int32 GetCurrentPOIIndex() const { return CurrentPOIIndex; }

    UFUNCTION(BlueprintPure, Category = "Gallery|POI")
    int32 GetPOICount() const;

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

    // Slower blend for flying in to / out of a POI
    UPROPERTY(EditDefaultsOnly, Category = "Gallery")
    float POIBlendTime = 1.2f;

    EGalleryMode PreviousMode = EGalleryMode::Walk;
    EGalleryMode ModeBeforePOI = EGalleryMode::TopView;

    // Standing spot on the floor in front of a POI, facing it (for "Walk here")
    bool GetWalkTransformInFrontOfPOI(const APOITarget* POI, FTransform& OutTransform) const;

    UPROPERTY()
    TArray<TObjectPtr<APOITarget>> POITargets;

    int32 CurrentPOIIndex = 0;

    // Collects every APOITarget in the level (called before the first pawn spawns)
    void GatherPOIs();

    // Where the walk pawn was when we left Walk mode, so returning puts you back there
    FTransform LastWalkTransform;
    bool bHasLastWalkTransform = false;

    UClass* GetPawnClassForMode(EGalleryMode Mode) const;
    FTransform GetSpawnTransformForMode(EGalleryMode Mode) const;
    APawn* SpawnPawnForMode(EGalleryMode Mode, AController* ForController);
    void ChangePawnForMode(EGalleryMode Mode);
};
