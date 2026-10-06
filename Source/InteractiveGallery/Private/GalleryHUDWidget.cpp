#include "GalleryHUDWidget.h"
#include "POITarget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "GalleryHUD"

// ---------------------------------------------------------------- UGalleryButton

UGalleryButton::UGalleryButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitIsFocusable(false);  // keep keyboard focus in the game, or arrows/Tab would navigate the UI
}

void UGalleryButton::SetClickAction(TFunction<void()> InAction)
{
    ClickAction = MoveTemp(InAction);
    OnClicked.AddUniqueDynamic(this, &UGalleryButton::HandleClicked);
}

void UGalleryButton::HandleClicked()
{
    if (ClickAction)
    {
        ClickAction();
    }
}

// ---------------------------------------------------------------- Lifecycle

void UGalleryHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // A Widget Blueprint subclass with its own designer layout keeps it
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildLayout();
    }
}

void UGalleryHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (AGalleryGameModeBase* GameMode = GetGalleryGameMode())
    {
        GameMode->OnModeSwitched.AddUniqueDynamic(this, &UGalleryHUDWidget::HandleModeSwitched);
        GameMode->OnPOIChanged.AddUniqueDynamic(this, &UGalleryHUDWidget::HandlePOIChanged);
    }

    Refresh();
}

void UGalleryHUDWidget::NativeDestruct()
{
    if (AGalleryGameModeBase* GameMode = GetGalleryGameMode())
    {
        GameMode->OnModeSwitched.RemoveDynamic(this, &UGalleryHUDWidget::HandleModeSwitched);
        GameMode->OnPOIChanged.RemoveDynamic(this, &UGalleryHUDWidget::HandlePOIChanged);
    }
    UnbindPOIContents();

    Super::NativeDestruct();
}

void UGalleryHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Keep the Top view markers pinned above their objects (also during the camera blend)
    AGalleryGameModeBase* GameMode = GetGalleryGameMode();
    APlayerController* PC = GetOwningPlayer();
    const bool bShowMarkers = GameMode && PC && GameMode->CurrentMode == EGalleryMode::TopView;

    for (int32 Index = 0; Index < MarkerButtons.Num(); ++Index)
    {
        UGalleryButton* Marker = MarkerButtons[Index];
        const APOITarget* POI = GameMode ? GameMode->GetPOI(Index) : nullptr;

        FVector2D ScreenPosition = FVector2D::ZeroVector;
        const bool bOnScreen = bShowMarkers && POI
            && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, POI->GetMarkerLocation(), ScreenPosition, false);

        Marker->SetVisibility(bOnScreen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bOnScreen)
        {
            if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
            {
                MarkerSlot->SetPosition(ScreenPosition);
            }
        }
    }
}

AGalleryGameModeBase* UGalleryHUDWidget::GetGalleryGameMode() const
{
    return GetWorld() ? GetWorld()->GetAuthGameMode<AGalleryGameModeBase>() : nullptr;
}

// ---------------------------------------------------------------- Events

void UGalleryHUDWidget::HandleModeSwitched(EGalleryMode NewMode)
{
    Refresh();
}

void UGalleryHUDWidget::HandlePOIChanged(APOITarget* NewPOI)
{
    Refresh();
}

void UGalleryHUDWidget::HandleVariantChanged(UMaterialVariantComponent* Component, int32 NewIndex)
{
    // Only recolour: rebuilding here would destroy the swatch button that is still handling its click
    UpdateVariantHighlights();
}

void UGalleryHUDWidget::HandlePaintingStatus(AAIPaintingFrame* Frame, EPaintingStatus Status)
{
    UpdatePaintingStatus();
}

// ---------------------------------------------------------------- Refresh

void UGalleryHUDWidget::Refresh()
{
    AGalleryGameModeBase* GameMode = GetGalleryGameMode();
    if (!GameMode)
    {
        return;
    }

    const EGalleryMode Mode = GameMode->CurrentMode;

    for (const TPair<EGalleryMode, TObjectPtr<UGalleryButton>>& Pair : ModeButtons)
    {
        Pair.Value->SetBackgroundColor(Pair.Key == Mode ? SelectedColor : ButtonColor);
    }

    if (ModeBarPanel)
    {
        // The POI panel has its own Top view / Walk here buttons
        ModeBarPanel->SetVisibility(Mode == EGalleryMode::POI ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    if (HintText)
    {
        switch (Mode)
        {
        case EGalleryMode::Walk:
            HintText->SetText(LOCTEXT("HintWalk", "WASD move  |  Mouse look  |  Space jump  |  Hold Alt for cursor  |  2 or Tab: top view"));
            break;
        case EGalleryMode::TopView:
            HintText->SetText(LOCTEXT("HintTop", "Click a + marker to visit an object  |  Drag: rotate  |  Wheel: zoom  |  1 or Tab: walk"));
            break;
        case EGalleryMode::POI:
            HintText->SetText(LOCTEXT("HintPOI", "Drag: orbit  |  Wheel: zoom  |  Left / Right: neighbouring objects  |  Backspace or Tab: back"));
            break;
        }
    }

    if (MarkerButtons.Num() != GameMode->GetPOICount())
    {
        RebuildMarkers();
    }

    APOITarget* POI = Mode == EGalleryMode::POI ? GameMode->GetCurrentPOI() : nullptr;

    if (POIPanel)
    {
        POIPanel->SetVisibility(POI ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (POI)
    {
        if (POINameText)
        {
            POINameText->SetText(POI->DisplayName.IsEmpty() ? FText::FromString(POI->GetActorNameOrLabel()) : POI->DisplayName);
        }
        if (POICounterText)
        {
            POICounterText->SetText(FText::Format(LOCTEXT("POICounter", "{0} / {1}"),
                GameMode->GetCurrentPOIIndex() + 1, GameMode->GetPOICount()));
        }
        if (POIDescriptionText)
        {
            POIDescriptionText->SetText(POI->Description);
            POIDescriptionText->SetVisibility(POI->Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        }
    }

    if (POI != DisplayedPOI)
    {
        RebuildPOIContents(POI);
    }
}

void UGalleryHUDWidget::RebuildMarkers()
{
    for (UGalleryButton* Marker : MarkerButtons)
    {
        Marker->RemoveFromParent();
    }
    MarkerButtons.Reset();

    AGalleryGameModeBase* GameMode = GetGalleryGameMode();
    if (!RootCanvas || !GameMode)
    {
        return;
    }

    for (int32 Index = 0; Index < GameMode->GetPOICount(); ++Index)
    {
        const APOITarget* POI = GameMode->GetPOI(Index);
        const FText Name = POI->DisplayName.IsEmpty() ? FText::FromString(POI->GetActorNameOrLabel()) : POI->DisplayName;

        UGalleryButton* Marker = MakeButton(FText::Format(LOCTEXT("Marker", "+  {0}"), Name), [this, Index]()
            {
                if (AGalleryGameModeBase* GM = GetGalleryGameMode())
                {
                    GM->GoToPOI(Index);
                }
            });
        Marker->SetBackgroundColor(MarkerColor);
        Marker->SetVisibility(ESlateVisibility::Collapsed);

        UCanvasPanelSlot* MarkerSlot = RootCanvas->AddChildToCanvas(Marker);
        MarkerSlot->SetAutoSize(true);
        MarkerSlot->SetAlignment(FVector2D(0.5f, 1.f));  // sits just above the point
        MarkerSlot->SetZOrder(-1);                       // under the panels
        MarkerButtons.Add(Marker);
    }
}

void UGalleryHUDWidget::RebuildPOIContents(APOITarget* POI)
{
    DisplayedPOI = POI;
    UnbindPOIContents();
    VariantButtons.Reset();

    if (VariantList)
    {
        VariantList->ClearChildren();
    }

    // Material swatches, one row per switchable target
    if (POI && VariantList)
    {
        for (UMaterialVariantComponent* Component : POI->GetMaterialVariantComponents())
        {
            if (Component->Variants.Num() == 0)
            {
                continue;
            }

            const FText Label = Component->SlotLabel.IsEmpty()
                ? FText::FromString(GetNameSafe(Component->GetOwner()))
                : Component->SlotLabel;
            VariantList->AddChildToVerticalBox(MakeText(Label, FontSize - 1))->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));

            UWrapBox* Row = WidgetTree->ConstructWidget<UWrapBox>();
            for (int32 Index = 0; Index < Component->Variants.Num(); ++Index)
            {
                TWeakObjectPtr<UMaterialVariantComponent> WeakComponent = Component;
                UGalleryButton* Swatch = MakeSwatchButton(Component->Variants[Index], [WeakComponent, Index]()
                    {
                        if (WeakComponent.IsValid())
                        {
                            WeakComponent->ApplyVariant(Index);
                        }
                    });
                Swatch->VariantComponent = Component;
                Swatch->VariantIndex = Index;
                VariantButtons.Add(Swatch);
                Row->AddChildToWrapBox(Swatch)->SetPadding(FMargin(3.f));
            }
            VariantList->AddChildToVerticalBox(Row);

            Component->OnVariantChanged.AddUniqueDynamic(this, &UGalleryHUDWidget::HandleVariantChanged);
            BoundVariantComponents.Add(Component);
        }
    }

    // AI painting: the first frame among the POI's targets
    DisplayedFrame = nullptr;
    if (POI)
    {
        for (AActor* Target : POI->MaterialTargets)
        {
            if (AAIPaintingFrame* Frame = Cast<AAIPaintingFrame>(Target))
            {
                DisplayedFrame = Frame;
                break;
            }
        }
    }

    if (PaintingSection)
    {
        PaintingSection->SetVisibility(DisplayedFrame ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (DisplayedFrame)
    {
        DisplayedFrame->OnStatusChanged.AddUniqueDynamic(this, &UGalleryHUDWidget::HandlePaintingStatus);
        if (PromptBox)
        {
            PromptBox->SetText(FText::FromString(DisplayedFrame->GetLastPrompt()));
        }
    }

    if (POI && VariantList && BoundVariantComponents.Num() == 0 && !DisplayedFrame)
    {
        UTextBlock* Empty = MakeText(LOCTEXT("NoVariants", "No material options here."), FontSize - 2);
        Empty->SetColorAndOpacity(FSlateColor(TextColor.CopyWithNewOpacity(0.6f)));
        VariantList->AddChildToVerticalBox(Empty)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
    }

    UpdateVariantHighlights();
    UpdatePaintingStatus();
}

void UGalleryHUDWidget::UpdateVariantHighlights()
{
    for (UGalleryButton* Button : VariantButtons)
    {
        const bool bSelected = Button->VariantComponent.IsValid()
            && Button->VariantComponent->GetCurrentVariant() == Button->VariantIndex;
        Button->SetBackgroundColor(bSelected ? SelectedColor : ButtonColor);
    }
}

void UGalleryHUDWidget::UpdatePaintingStatus()
{
    if (!DisplayedFrame)
    {
        return;
    }

    const EPaintingStatus Status = DisplayedFrame->GetStatus();
    if (PaintStatusText)
    {
        PaintStatusText->SetText(DisplayedFrame->GetStatusText());
        PaintStatusText->SetColorAndOpacity(FSlateColor(Status == EPaintingStatus::Failed
            ? FLinearColor(1.f, 0.45f, 0.4f)
            : TextColor.CopyWithNewOpacity(0.8f)));
    }
    if (PaintButton)
    {
        PaintButton->SetIsEnabled(Status != EPaintingStatus::Working);
    }
}

void UGalleryHUDWidget::UnbindPOIContents()
{
    for (UMaterialVariantComponent* Component : BoundVariantComponents)
    {
        if (Component)
        {
            Component->OnVariantChanged.RemoveDynamic(this, &UGalleryHUDWidget::HandleVariantChanged);
        }
    }
    BoundVariantComponents.Reset();

    if (DisplayedFrame)
    {
        DisplayedFrame->OnStatusChanged.RemoveDynamic(this, &UGalleryHUDWidget::HandlePaintingStatus);
    }
}

void UGalleryHUDWidget::PaintCurrentFrame()
{
    if (DisplayedFrame && PromptBox)
    {
        DisplayedFrame->Paint(PromptBox->GetText().ToString());
    }

    // Hand the keyboard back to the game so Tab / arrows work again
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
    }
}

// ---------------------------------------------------------------- Layout

void UGalleryHUDWidget::BuildLayout()
{
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
    RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);  // let clicks through to the viewport
    WidgetTree->RootWidget = RootCanvas;

    // Mode bar, bottom centre
    UHorizontalBox* ModeBar = WidgetTree->ConstructWidget<UHorizontalBox>();
    const TPair<EGalleryMode, FText> Modes[] = {
        { EGalleryMode::Walk,    LOCTEXT("ModeWalk", "1  Walk") },
        { EGalleryMode::TopView, LOCTEXT("ModeTop", "2  Top view") },
    };
    for (const TPair<EGalleryMode, FText>& Mode : Modes)
    {
        const EGalleryMode TargetMode = Mode.Key;
        UGalleryButton* Button = MakeButton(Mode.Value, [this, TargetMode]()
            {
                AGalleryGameModeBase* GameMode = GetGalleryGameMode();
                if (GameMode && GameMode->CurrentMode != TargetMode)
                {
                    GameMode->SwitchMode(TargetMode);
                }
            });
        ModeBar->AddChildToHorizontalBox(Button)->SetPadding(FMargin(4.f, 0.f));
        ModeButtons.Add(TargetMode, Button);
    }

    ModeBarPanel = MakePanel(ModeBar);
    UCanvasPanelSlot* BarSlot = RootCanvas->AddChildToCanvas(ModeBarPanel);
    BarSlot->SetAnchors(FAnchors(0.5f, 1.f));
    BarSlot->SetAlignment(FVector2D(0.5f, 1.f));
    BarSlot->SetPosition(FVector2D(0.f, -24.f));
    BarSlot->SetAutoSize(true);

    // Controls hint, bottom left
    HintText = MakeText(FText::GetEmpty(), FontSize - 3);
    HintText->SetVisibility(ESlateVisibility::HitTestInvisible);
    HintText->SetColorAndOpacity(FSlateColor(TextColor.CopyWithNewOpacity(0.75f)));
    HintText->SetShadowOffset(FVector2D(1.f, 1.f));
    HintText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
    UCanvasPanelSlot* HintSlot = RootCanvas->AddChildToCanvas(HintText);
    HintSlot->SetAnchors(FAnchors(0.f, 1.f));
    HintSlot->SetAlignment(FVector2D(0.f, 1.f));
    HintSlot->SetPosition(FVector2D(24.f, -90.f));
    HintSlot->SetAutoSize(true);

    // POI panel, right side
    UVerticalBox* POIContent = WidgetTree->ConstructWidget<UVerticalBox>();

    UHorizontalBox* Nav = WidgetTree->ConstructWidget<UHorizontalBox>();
    UGalleryButton* Previous = MakeButton(LOCTEXT("PrevPOI", "<"), [this]()
        {
            if (AGalleryGameModeBase* GameMode = GetGalleryGameMode()) { GameMode->PreviousPOI(); }
        });
    UGalleryButton* Next = MakeButton(LOCTEXT("NextPOI", ">"), [this]()
        {
            if (AGalleryGameModeBase* GameMode = GetGalleryGameMode()) { GameMode->NextPOI(); }
        });
    POINameText = MakeText(FText::GetEmpty(), FontSize + 4);
    POINameText->SetJustification(ETextJustify::Center);

    Nav->AddChildToHorizontalBox(Previous)->SetVerticalAlignment(VAlign_Center);
    UHorizontalBoxSlot* NameSlot = Nav->AddChildToHorizontalBox(POINameText);
    NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    NameSlot->SetVerticalAlignment(VAlign_Center);
    NameSlot->SetPadding(FMargin(8.f, 0.f));
    Nav->AddChildToHorizontalBox(Next)->SetVerticalAlignment(VAlign_Center);
    POIContent->AddChildToVerticalBox(Nav);

    POICounterText = MakeText(FText::GetEmpty(), FontSize - 3);
    POICounterText->SetJustification(ETextJustify::Center);
    POICounterText->SetColorAndOpacity(FSlateColor(TextColor.CopyWithNewOpacity(0.6f)));
    POIContent->AddChildToVerticalBox(POICounterText)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

    POIDescriptionText = MakeText(FText::GetEmpty(), FontSize - 1, true);
    POIContent->AddChildToVerticalBox(POIDescriptionText)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

    VariantList = WidgetTree->ConstructWidget<UVerticalBox>();
    POIContent->AddChildToVerticalBox(VariantList);

    // AI painting section (shown for POIs with an AIPaintingFrame target)
    UVerticalBox* Painting = WidgetTree->ConstructWidget<UVerticalBox>();
    Painting->AddChildToVerticalBox(MakeText(LOCTEXT("PaintingHeader", "AI painting (ComfyUI)"), FontSize - 1))
        ->SetPadding(FMargin(0.f, 12.f, 0.f, 4.f));

    PromptBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>();
    PromptBox->SetHintText(LOCTEXT("PromptHint", "Describe a painting..."));
    USizeBox* PromptSize = WidgetTree->ConstructWidget<USizeBox>();
    PromptSize->SetHeightOverride(84.f);
    PromptSize->AddChild(PromptBox);
    Painting->AddChildToVerticalBox(PromptSize);

    PaintButton = MakeButton(LOCTEXT("Paint", "Paint"), [this]() { PaintCurrentFrame(); });
    PaintButton->SetBackgroundColor(SelectedColor);
    Painting->AddChildToVerticalBox(PaintButton)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

    PaintStatusText = MakeText(FText::GetEmpty(), FontSize - 3, true);
    Painting->AddChildToVerticalBox(PaintStatusText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

    PaintingSection = Painting;
    POIContent->AddChildToVerticalBox(Painting);

    // Leave the POI
    UHorizontalBox* Exits = WidgetTree->ConstructWidget<UHorizontalBox>();
    UGalleryButton* BackToTop = MakeButton(LOCTEXT("BackTop", "Top view"), [this]()
        {
            if (AGalleryGameModeBase* GameMode = GetGalleryGameMode()) { GameMode->SwitchMode(EGalleryMode::TopView); }
        });
    UGalleryButton* WalkHere = MakeButton(LOCTEXT("WalkHere", "Walk here"), [this]()
        {
            if (AGalleryGameModeBase* GameMode = GetGalleryGameMode()) { GameMode->SwitchMode(EGalleryMode::Walk); }
        });
    UHorizontalBoxSlot* TopSlot = Exits->AddChildToHorizontalBox(BackToTop);
    TopSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TopSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
    UHorizontalBoxSlot* WalkSlot = Exits->AddChildToHorizontalBox(WalkHere);
    WalkSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    WalkSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
    POIContent->AddChildToVerticalBox(Exits)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

    USizeBox* PanelWidth = WidgetTree->ConstructWidget<USizeBox>();
    PanelWidth->SetWidthOverride(340.f);
    PanelWidth->AddChild(POIContent);

    POIPanel = MakePanel(PanelWidth);
    UCanvasPanelSlot* POISlot = RootCanvas->AddChildToCanvas(POIPanel);
    POISlot->SetAnchors(FAnchors(1.f, 0.5f));
    POISlot->SetAlignment(FVector2D(1.f, 0.5f));
    POISlot->SetPosition(FVector2D(-24.f, 0.f));
    POISlot->SetAutoSize(true);
}

UGalleryButton* UGalleryHUDWidget::MakeButton(const FText& Label, TFunction<void()> OnClick)
{
    UGalleryButton* Button = WidgetTree->ConstructWidget<UGalleryButton>();
    Button->SetBackgroundColor(ButtonColor);
    Button->SetClickAction(MoveTemp(OnClick));

    UTextBlock* Text = MakeText(Label, FontSize);
    Text->SetJustification(ETextJustify::Center);
    UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->AddChild(Text));
    ContentSlot->SetPadding(FMargin(14.f, 6.f));
    return Button;
}

UGalleryButton* UGalleryHUDWidget::MakeSwatchButton(const FMaterialVariant& Variant, TFunction<void()> OnClick)
{
    UGalleryButton* Button = WidgetTree->ConstructWidget<UGalleryButton>();
    Button->SetBackgroundColor(ButtonColor);
    Button->SetClickAction(MoveTemp(OnClick));
    Button->SetToolTipText(Variant.DisplayName);

    UImage* Swatch = WidgetTree->ConstructWidget<UImage>();
    if (Variant.Thumbnail)
    {
        Swatch->SetBrushFromTexture(Variant.Thumbnail);
    }
    else
    {
        Swatch->SetColorAndOpacity(Variant.SwatchColor);
    }

    USizeBox* SwatchBox = WidgetTree->ConstructWidget<USizeBox>();
    SwatchBox->SetWidthOverride(SwatchSize);
    SwatchBox->SetHeightOverride(SwatchSize);
    SwatchBox->AddChild(Swatch);

    UTextBlock* Label = MakeText(Variant.DisplayName, FontSize - 4);
    Label->SetJustification(ETextJustify::Center);

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Content->AddChildToVerticalBox(SwatchBox)->SetHorizontalAlignment(HAlign_Center);
    Content->AddChildToVerticalBox(Label)->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));

    UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->AddChild(Content));
    ContentSlot->SetPadding(FMargin(4.f));
    return Button;
}

UTextBlock* UGalleryHUDWidget::MakeText(const FText& Text, int32 Size, bool bWrap)
{
    UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
    Block->SetText(Text);
    Block->SetColorAndOpacity(FSlateColor(TextColor));

    FSlateFontInfo Font = Block->GetFont();
    Font.Size = Size;
    Block->SetFont(Font);

    if (bWrap)
    {
        Block->SetAutoWrapText(true);
    }
    return Block;
}

UBorder* UGalleryHUDWidget::MakePanel(UWidget* Content)
{
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(PanelColor);
    Panel->SetPadding(FMargin(12.f));
    Panel->SetContent(Content);
    return Panel;
}

#undef LOCTEXT_NAMESPACE
