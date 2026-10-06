#include "TopViewPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GalleryGameModeBase.h"

ATopViewPawn::ATopViewPawn()
{
    SpringArm->TargetArmLength = 1800.f;
    SpringArm->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bUsePawnControlRotation = false;

    MinArmLength = 400.f;
    MaxArmLength = 3500.f;
    MouseOrbitSensitivity = 0.4f;

    UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] Created with Enhanced Input setup"));
}

void ATopViewPawn::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] %s active - Top view ready"), *GetName());
}

void ATopViewPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] Setting up Enhanced Input for TopView"));

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                Subsystem->ClearAllMappings();

                if (UInputMappingContext* LoadedContext = TopViewMappingContext.LoadSynchronous())
                {
                    Subsystem->AddMappingContext(LoadedContext, 0);
                    CacheHeldKeys(LoadedContext);
                    UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] Added TopView Input Mapping Context"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[TopViewPawn] Failed to load TopView Input Mapping Context!"));
                }
            }
        }

        // Bind TopView actions
        if (UInputAction* LoadedRotate = TopViewRotateAction.LoadSynchronous())
        {
            EnhancedInputComponent->BindAction(LoadedRotate, ETriggerEvent::Triggered, this, &ATopViewPawn::RotateCamera);
            UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] Bound Rotate Action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[TopViewPawn] Failed to load Rotate Action!"));
        }

        if (UInputAction* LoadedZoom = TopViewZoomAction.LoadSynchronous())
        {
            EnhancedInputComponent->BindAction(LoadedZoom, ETriggerEvent::Triggered, this, &ATopViewPawn::ZoomCamera);
            UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] Bound Zoom Action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[TopViewPawn] Failed to load Zoom Action!"));
        }

        // Bind Switch Action
        if (UInputAction* LoadedSwitch = SwitchAction.LoadSynchronous())
        {
            EnhancedInputComponent->BindAction(LoadedSwitch, ETriggerEvent::Started, this, &ATopViewPawn::SwitchModePressed);
            UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] Bound Switch Action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[TopViewPawn] Failed to load Switch Action!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[TopViewPawn] Failed to get EnhancedInputComponent!"));
    }
}

void ATopViewPawn::RotateCamera(const FInputActionValue& Value)
{
    const float Delta = GetOrbitDelta(Value, TopViewRotateAction.Get());
    if (Delta != 0.f)
    {
        FRotator NewRotation = SpringArm->GetRelativeRotation();
        NewRotation.Yaw += Delta;
        SpringArm->SetRelativeRotation(NewRotation);
    }
}

void ATopViewPawn::ZoomCamera(const FInputActionValue& Value)
{
    ApplyZoomInput(Value, TopViewZoomAction.Get());
}

// Switch Mode function
void ATopViewPawn::SwitchModePressed()
{
    UE_LOG(LogTemp, Display, TEXT("[TopViewPawn] Switch Mode key pressed"));

    if (AGalleryGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGalleryGameModeBase>())
    {
        // Next tick: switching destroys this pawn, which must not happen inside its own input callback
        GetWorldTimerManager().SetTimerForNextTick(GameMode, &AGalleryGameModeBase::CycleMode);
    }
}
