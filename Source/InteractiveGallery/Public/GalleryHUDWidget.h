#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "AIPaintingFrame.h"
#include "GalleryGameModeBase.h"
#include "MaterialVariantComponent.h"
#include "GalleryHUDWidget.generated.h"

class APOITarget;
class UBorder;
class UCanvasPanel;
class UMultiLineEditableTextBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

// Button that runs a native callback, so the code-built HUD can create buttons on the fly
UCLASS()
class INTERACTIVEGALLERY_API UGalleryButton : public UButton
{
    GENERATED_BODY()

public:
    UGalleryButton(const FObjectInitializer& ObjectInitializer);

    void SetClickAction(TFunction<void()> InAction);

    // Set on material swatch buttons so the HUD can highlight the active variant
    TWeakObjectPtr<UMaterialVariantComponent> VariantComponent;
    int32 VariantIndex = INDEX_NONE;

private:
    TFunction<void()> ClickAction;

    UFUNCTION()
    void HandleClicked();
};

// Code-built HUD:
//   Walk / Top view   mode bar (bottom) + controls hint
//   Top view          a clickable marker above every POI target
//   POI               panel (right): name, description, material swatches, AI prompt box for
//                     painting frames, and Top view / Walk here buttons
// A Widget Blueprint subclass with its own designer layout skips the code-built tree.
UCLASS()
class INTERACTIVEGALLERY_API UGalleryHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Gallery")
    void Refresh();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(EditDefaultsOnly, Category = "Style")
    FLinearColor PanelColor = FLinearColor(0.01f, 0.012f, 0.02f, 0.7f);

    UPROPERTY(EditDefaultsOnly, Category = "Style")
    FLinearColor ButtonColor = FLinearColor(0.12f, 0.13f, 0.16f, 0.95f);

    UPROPERTY(EditDefaultsOnly, Category = "Style")
    FLinearColor SelectedColor = FLinearColor(0.95f, 0.55f, 0.12f, 1.f);

    UPROPERTY(EditDefaultsOnly, Category = "Style")
    FLinearColor MarkerColor = FLinearColor(0.95f, 0.55f, 0.12f, 0.9f);

    UPROPERTY(EditDefaultsOnly, Category = "Style")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, Category = "Style")
    int32 FontSize = 14;

    UPROPERTY(EditDefaultsOnly, Category = "Style")
    float SwatchSize = 56.f;

private:
    UFUNCTION()
    void HandleModeSwitched(EGalleryMode NewMode);

    UFUNCTION()
    void HandlePOIChanged(APOITarget* NewPOI);

    UFUNCTION()
    void HandleVariantChanged(UMaterialVariantComponent* Component, int32 NewIndex);

    UFUNCTION()
    void HandlePaintingStatus(AAIPaintingFrame* Frame, EPaintingStatus Status);

    void BuildLayout();
    void RebuildMarkers();
    void RebuildPOIContents(APOITarget* POI);
    void UpdateVariantHighlights();
    void UpdatePaintingStatus();
    void UnbindPOIContents();
    void PaintCurrentFrame();
    AGalleryGameModeBase* GetGalleryGameMode() const;

    UGalleryButton* MakeButton(const FText& Label, TFunction<void()> OnClick);
    UGalleryButton* MakeSwatchButton(const FMaterialVariant& Variant, TFunction<void()> OnClick);
    UTextBlock* MakeText(const FText& Text, int32 Size, bool bWrap = false);
    UBorder* MakePanel(UWidget* Content);

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UWidget> ModeBarPanel;

    UPROPERTY()
    TMap<EGalleryMode, TObjectPtr<UGalleryButton>> ModeButtons;

    UPROPERTY()
    TObjectPtr<UTextBlock> HintText;

    // One per POI target, same order as the game mode's POI list
    UPROPERTY()
    TArray<TObjectPtr<UGalleryButton>> MarkerButtons;

    UPROPERTY()
    TObjectPtr<UWidget> POIPanel;

    UPROPERTY()
    TObjectPtr<UTextBlock> POINameText;

    UPROPERTY()
    TObjectPtr<UTextBlock> POICounterText;

    UPROPERTY()
    TObjectPtr<UTextBlock> POIDescriptionText;

    UPROPERTY()
    TObjectPtr<UVerticalBox> VariantList;

    UPROPERTY()
    TArray<TObjectPtr<UGalleryButton>> VariantButtons;

    UPROPERTY()
    TArray<TObjectPtr<UMaterialVariantComponent>> BoundVariantComponents;

    UPROPERTY()
    TObjectPtr<UWidget> PaintingSection;

    UPROPERTY()
    TObjectPtr<UMultiLineEditableTextBox> PromptBox;

    UPROPERTY()
    TObjectPtr<UGalleryButton> PaintButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> PaintStatusText;

    UPROPERTY()
    TObjectPtr<AAIPaintingFrame> DisplayedFrame;

    UPROPERTY()
    TObjectPtr<APOITarget> DisplayedPOI;
};
