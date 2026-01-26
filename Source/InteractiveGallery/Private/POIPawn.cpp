#include "POIPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GalleryGameModeBase.h"

APOIPawn::APOIPawn()
{
    SpringArm->TargetArmLength = 300.f;
    SpringArm->bDoCollisionTest = false;
    SpringArm->bUsePawnControlRotation = false;

    UE_LOG(LogTemp, Display, TEXT("[POIPawn] Created with Enhanced Input setup"));
}

void APOIPawn::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }

    UE_LOG(LogTemp, Display, TEXT("[POIPawn] %s active - Close-up view ready"), *GetName());
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
    float AxisValue = Value.Get<float>();

    if (FMath::Abs(AxisValue) > 0.1f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[POIPawn] ROTATE INPUT: %.2f"), AxisValue);
    }

    if (FMath::Abs(AxisValue) > 0.01f)
    {
        FRotator NewRotation = GetActorRotation();
        NewRotation.Yaw += AxisValue * RotationSpeed * GetWorld()->GetDeltaSeconds();
        SetActorRotation(NewRotation);
    }
}

void APOIPawn::ZoomPOI(const FInputActionValue& Value)
{
    float AxisValue = Value.Get<float>();

    if (FMath::Abs(AxisValue) > 0.1f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[POIPawn] ZOOM INPUT: %.2f"), AxisValue);
    }

    if (FMath::Abs(AxisValue) > 0.01f)
    {
        float CurrentLength = SpringArm->TargetArmLength;
        float NewLength = CurrentLength - AxisValue * ZoomSpeed * GetWorld()->GetDeltaSeconds();
        NewLength = FMath::Clamp(NewLength, 100.0f, 500.0f);
        SpringArm->TargetArmLength = NewLength;
    }
}

// Switch Mode function
void APOIPawn::SwitchModePressed()
{
    UE_LOG(LogTemp, Display, TEXT("[POIPawn] Switch Mode key pressed"));

    if (UWorld* World = GetWorld())
    {
        if (AGalleryGameModeBase* GameMode = Cast<AGalleryGameModeBase>(World->GetAuthGameMode()))
        {
            switch (GameMode->CurrentMode)
            {
            case EGalleryMode::POI:
                GameMode->SwitchMode(EGalleryMode::Walk);
                break;
            case EGalleryMode::Walk:
                GameMode->SwitchMode(EGalleryMode::TopView);
                break;
            case EGalleryMode::TopView:
                GameMode->SwitchMode(EGalleryMode::POI);
                break;
            }
        }
    }
}