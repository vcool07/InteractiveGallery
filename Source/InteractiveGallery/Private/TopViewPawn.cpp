#include "TopViewPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GalleryGameModeBase.h"

ATopViewPawn::ATopViewPawn()
{
    SpringArm->TargetArmLength = 1000.f;
    SpringArm->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bUsePawnControlRotation = false;

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
    float AxisValue = Value.Get<float>();

    if (FMath::Abs(AxisValue) > 0.01f)
    {
        FRotator NewRotation = SpringArm->GetRelativeRotation();
        NewRotation.Yaw += AxisValue * RotationSpeed * GetWorld()->GetDeltaSeconds();
        SpringArm->SetRelativeRotation(NewRotation);
    }
}

void ATopViewPawn::ZoomCamera(const FInputActionValue& Value)
{
    float AxisValue = Value.Get<float>();

    if (FMath::Abs(AxisValue) > 0.01f)
    {
        float CurrentLength = SpringArm->TargetArmLength;
        float NewLength = CurrentLength - AxisValue * ZoomSpeed * GetWorld()->GetDeltaSeconds();
        NewLength = FMath::Clamp(NewLength, 500.0f, 2000.0f);
        SpringArm->TargetArmLength = NewLength;
    }
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
