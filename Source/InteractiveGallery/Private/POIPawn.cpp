#include "POIPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GalleryGameModeBase.h"
#include "POITarget.h"

APOIPawn::APOIPawn()
{
    SpringArm->TargetArmLength = 300.f;
    SpringArm->bDoCollisionTest = false;
    SpringArm->bUsePawnControlRotation = false;

    MinArmLength = 100.f;
    MaxArmLength = 500.f;

    UE_LOG(LogTemp, Display, TEXT("[POIPawn] Created with Enhanced Input setup"));
}

void APOIPawn::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Display, TEXT("[POIPawn] %s active - Close-up view ready"), *GetName());
}

void APOIPawn::FocusOn(APOITarget* Target, bool bAnimate)
{
    if (!Target)
    {
        return;
    }

    SetArmLengthLimits(Target->MinViewDistance, Target->MaxViewDistance);
    SetDesiredArmLength(Target->ViewDistance, !bAnimate);

    const FTransform Pivot(Target->GetActorRotation(), Target->GetActorLocation());
    if (bAnimate)
    {
        StartCameraTransitionToTransform(Pivot, FocusTransitionTime);
    }
    else
    {
        SetActorTransform(Pivot);
    }

    UE_LOG(LogTemp, Display, TEXT("[POIPawn] Focusing on %s"), *Target->DisplayName.ToString());
}

void APOIPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogTemp, Display, TEXT("[POIPawn] Setting up Enhanced Input for POI"));

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                Subsystem->ClearAllMappings();

                if (UInputMappingContext* LoadedContext = POIMappingContext.LoadSynchronous())
                {
                    Subsystem->AddMappingContext(LoadedContext, 0);
                    CacheHeldKeys(LoadedContext);
                    UE_LOG(LogTemp, Display, TEXT("[POIPawn] Added POI Input Mapping Context"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[POIPawn] Failed to load POI Input Mapping Context!"));
                }
            }
        }

        // Bind POI actions
        if (UInputAction* LoadedRotate = POIRotateAction.LoadSynchronous())
        {
            EnhancedInputComponent->BindAction(LoadedRotate, ETriggerEvent::Triggered, this, &APOIPawn::RotateAroundPOI);
            UE_LOG(LogTemp, Display, TEXT("[POIPawn] Bound Rotate Action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[POIPawn] Failed to load Rotate Action!"));
        }

        if (UInputAction* LoadedZoom = POIZoomAction.LoadSynchronous())
        {
            EnhancedInputComponent->BindAction(LoadedZoom, ETriggerEvent::Triggered, this, &APOIPawn::ZoomPOI);
            UE_LOG(LogTemp, Display, TEXT("[POIPawn] Bound Zoom Action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[POIPawn] Failed to load Zoom Action!"));
        }

        // Bind Switch Action
        if (UInputAction* LoadedSwitch = SwitchAction.LoadSynchronous())
        {
            EnhancedInputComponent->BindAction(LoadedSwitch, ETriggerEvent::Started, this, &APOIPawn::SwitchModePressed);
            UE_LOG(LogTemp, Display, TEXT("[POIPawn] Bound Switch Action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[POIPawn] Failed to load Switch Action!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[POIPawn] Failed to get EnhancedInputComponent!"));
    }
}

void APOIPawn::RotateAroundPOI(const FInputActionValue& Value)
{
    // Don't fight the fly-to animation
    if (CameraTransition->IsTransitioning())
    {
        return;
    }

    const float Delta = GetOrbitDelta(Value, POIRotateAction.Get());
    if (Delta != 0.f)
    {
        FRotator NewRotation = GetActorRotation();
        NewRotation.Yaw += Delta;
        SetActorRotation(NewRotation);
    }
}

void APOIPawn::ZoomPOI(const FInputActionValue& Value)
{
    ApplyZoomInput(Value, POIZoomAction.Get());
}

// Switch Mode function
void APOIPawn::SwitchModePressed()
{
    UE_LOG(LogTemp, Display, TEXT("[POIPawn] Switch Mode key pressed"));

    if (AGalleryGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGalleryGameModeBase>())
    {
        // Next tick: switching destroys this pawn, which must not happen inside its own input callback
        GetWorldTimerManager().SetTimerForNextTick(GameMode, &AGalleryGameModeBase::CycleMode);
    }
}
