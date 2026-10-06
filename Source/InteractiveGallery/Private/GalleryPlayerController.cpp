#include "GalleryPlayerController.h"
#include "GalleryHUDWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AGalleryPlayerController::AGalleryPlayerController()
{
    HUDWidgetClass = UGalleryHUDWidget::StaticClass();
}

void AGalleryPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalPlayerController() && HUDWidgetClass)
    {
        HUDWidget = CreateWidget<UGalleryHUDWidget>(this, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport();
            UE_LOG(LogTemp, Display, TEXT("[PlayerController] HUD created"));
        }
    }
}

void AGalleryPlayerController::CreateUIInput()
{
    auto MakeAction = [this](const TCHAR* Name, FKey Key)
    {
        UInputAction* Action = NewObject<UInputAction>(this, Name);
        UIMappingContext->MapKey(Action, Key);
        return Action;
    };

    UIMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_GalleryUI"));
    WalkModeAction = MakeAction(TEXT("IA_UI_WalkMode"), EKeys::One);
    TopViewModeAction = MakeAction(TEXT("IA_UI_TopViewMode"), EKeys::Two);
    BackAction = MakeAction(TEXT("IA_UI_Back"), EKeys::BackSpace);
    NextPOIAction = MakeAction(TEXT("IA_UI_NextPOI"), EKeys::Right);
    PreviousPOIAction = MakeAction(TEXT("IA_UI_PreviousPOI"), EKeys::Left);
    ShowCursorAction = MakeAction(TEXT("IA_UI_ShowCursor"), EKeys::LeftAlt);
}

void AGalleryPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    CreateUIInput();

    if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
    {
        Input->BindAction(WalkModeAction, ETriggerEvent::Started, this, &AGalleryPlayerController::OnWalkModeKey);
        Input->BindAction(TopViewModeAction, ETriggerEvent::Started, this, &AGalleryPlayerController::OnTopViewModeKey);
        Input->BindAction(BackAction, ETriggerEvent::Started, this, &AGalleryPlayerController::OnBackKey);
        Input->BindAction(NextPOIAction, ETriggerEvent::Started, this, &AGalleryPlayerController::OnNextPOIKey);
        Input->BindAction(PreviousPOIAction, ETriggerEvent::Started, this, &AGalleryPlayerController::OnPreviousPOIKey);
        Input->BindAction(ShowCursorAction, ETriggerEvent::Started, this, &AGalleryPlayerController::OnShowCursorPressed);
        Input->BindAction(ShowCursorAction, ETriggerEvent::Completed, this, &AGalleryPlayerController::OnShowCursorReleased);
    }

    AddUIMappingContext();
}

void AGalleryPlayerController::OnPossess(APawn* InPawn)
{
    // The new pawn sets its own cursor/input mode in PossessedBy, so drop the Walk-mode cursor override
    if (bWalkCursorActive)
    {
        bWalkCursorActive = false;
        ResetIgnoreLookInput();
    }

    Super::OnPossess(InPawn);

    AddUIMappingContext();
}

void AGalleryPlayerController::AddUIMappingContext()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (UIMappingContext && !Subsystem->HasMappingContext(UIMappingContext))
        {
            Subsystem->AddMappingContext(UIMappingContext, 1);
        }
    }
}

AGalleryGameModeBase* AGalleryPlayerController::GetGalleryGameMode() const
{
    return GetWorld() ? GetWorld()->GetAuthGameMode<AGalleryGameModeBase>() : nullptr;
}

void AGalleryPlayerController::RequestMode(EGalleryMode Mode)
{
    // Next tick: switching destroys the pawn, whose input is still being processed this frame
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Mode]()
        {
            if (AGalleryGameModeBase* GameMode = GetGalleryGameMode())
            {
                if (GameMode->CurrentMode != Mode)
                {
                    GameMode->SwitchMode(Mode);
                }
            }
        }));
}

void AGalleryPlayerController::OnBackKey()
{
    AGalleryGameModeBase* GameMode = GetGalleryGameMode();
    if (GameMode && GameMode->CurrentMode == EGalleryMode::POI)
    {
        RequestMode(GameMode->GetModeBeforePOI());
    }
}

void AGalleryPlayerController::OnNextPOIKey()
{
    AGalleryGameModeBase* GameMode = GetGalleryGameMode();
    if (GameMode && GameMode->CurrentMode == EGalleryMode::POI)
    {
        GameMode->NextPOI();
    }
}

void AGalleryPlayerController::OnPreviousPOIKey()
{
    AGalleryGameModeBase* GameMode = GetGalleryGameMode();
    if (GameMode && GameMode->CurrentMode == EGalleryMode::POI)
    {
        GameMode->PreviousPOI();
    }
}

void AGalleryPlayerController::OnShowCursorPressed()
{
    AGalleryGameModeBase* GameMode = GetGalleryGameMode();
    if (!GameMode || GameMode->CurrentMode != EGalleryMode::Walk || bWalkCursorActive)
    {
        return;
    }

    bWalkCursorActive = true;
    bShowMouseCursor = true;
    SetInputMode(FInputModeGameAndUI());
    SetIgnoreLookInput(true);  // otherwise moving the cursor would also turn the camera
}

void AGalleryPlayerController::OnShowCursorReleased()
{
    if (!bWalkCursorActive)
    {
        return;
    }

    bWalkCursorActive = false;
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
    ResetIgnoreLookInput();
}
