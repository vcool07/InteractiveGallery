#include "WalkCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GalleryGameModeBase.h"

AWalkCharacter::AWalkCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    GetMesh()->SetOwnerNoSee(true);

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = 600.f;
        Movement->JumpZVelocity = 420.f;
    }

    UE_LOG(LogTemp, Display, TEXT("[WalkCharacter] Created with Enhanced Input ready"));
}

void AWalkCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AWalkCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogTemp, Display, TEXT("[WalkCharacter] Setting up ENHANCED INPUT"));

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                Subsystem->ClearAllMappings();

                if (UInputMappingContext* LoadedContext = InputMapping.LoadSynchronous())
                {
                    Subsystem->AddMappingContext(LoadedContext, 0);
                    UE_LOG(LogTemp, Display, TEXT("[WalkCharacter] Added Input Mapping Context"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[WalkCharacter] Failed to load Input Mapping Context!"));
                }
            }
        }

        // Bind actions
        if (UInputAction* LoadedMove = MoveAction.LoadSynchronous())
            EnhancedInputComponent->BindAction(LoadedMove, ETriggerEvent::Triggered, this, &AWalkCharacter::Move);

        if (UInputAction* LoadedLook = LookAction.LoadSynchronous())
            EnhancedInputComponent->BindAction(LoadedLook, ETriggerEvent::Triggered, this, &AWalkCharacter::Look);

        if (UInputAction* LoadedJump = JumpAction.LoadSynchronous())
            EnhancedInputComponent->BindAction(LoadedJump, ETriggerEvent::Started, this, &AWalkCharacter::JumpPressed);

        if (UInputAction* LoadedSwitch = SwitchAction.LoadSynchronous())
            EnhancedInputComponent->BindAction(LoadedSwitch, ETriggerEvent::Started, this, &AWalkCharacter::SwitchModePressed);
    }
}

void AWalkCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    // DEBUG LOG
    if (MovementVector.SizeSquared() > 0.01f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WalkCharacter] MOVE INPUT: Forward=%.2f, Right=%.2f"),
            MovementVector.Y, MovementVector.X);
    }

    if (Controller)
    {
        AddMovementInput(GetActorForwardVector(), MovementVector.Y);
        AddMovementInput(GetActorRightVector(), MovementVector.X);
    }
}

void AWalkCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookVector = Value.Get<FVector2D>();

    if (Controller)
    {
        AddControllerYawInput(LookVector.X);
        AddControllerPitchInput(LookVector.Y);
    }
}

void AWalkCharacter::JumpPressed()
{
    ACharacter::Jump();
    UE_LOG(LogTemp, Display, TEXT("[WalkCharacter] Jump pressed"));
}

void AWalkCharacter::SwitchModePressed()
{
    UE_LOG(LogTemp, Display, TEXT("[WalkCharacter] Switch Mode key pressed"));

    if (UWorld* World = GetWorld())
    {
        if (AGalleryGameModeBase* GameMode = Cast<AGalleryGameModeBase>(World->GetAuthGameMode()))
        {
            switch (GameMode->CurrentMode)
            {
            case EGalleryMode::Walk:
                GameMode->SwitchMode(EGalleryMode::TopView);
                break;
            case EGalleryMode::TopView:
                GameMode->SwitchMode(EGalleryMode::POI);
                break;
            case EGalleryMode::POI:
                GameMode->SwitchMode(EGalleryMode::Walk);
                break;
            }
        }
    }
}