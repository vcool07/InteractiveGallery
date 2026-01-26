#include "GalleryGameModeBase.h"
#include "WalkCharacter.h"
#include "TopViewPawn.h"
#include "POIPawn.h"
#include "Engine/Engine.h"

AGalleryGameModeBase::AGalleryGameModeBase()
{
    CurrentMode = EGalleryMode::Walk;
    CurrentPawn = nullptr;

    WalkPawnClass = AWalkCharacter::StaticClass();
    TopViewPawnClass = ATopViewPawn::StaticClass();
    POIPawnClass = APOIPawn::StaticClass();

    UE_LOG(LogTemp, Display, TEXT("[GameMode] Initialized with C++ pawn classes"));
}

void AGalleryGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Display, TEXT("[GameMode] BeginPlay - Starting game"));

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
        UE_LOG(LogTemp, Display, TEXT("[GameMode] Mouse cursor hidden for game input"));
    }

    ChangePawnForMode(CurrentMode);
}

void AGalleryGameModeBase::SwitchMode(EGalleryMode NewMode)
{
    if (NewMode == CurrentMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] SwitchMode ignored: Already in %s"),
            *UEnum::GetValueAsString(NewMode));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[GameMode] Switching: %s -> %s"),
        *UEnum::GetValueAsString(CurrentMode),
        *UEnum::GetValueAsString(NewMode));

    CurrentMode = NewMode;
    ChangePawnForMode(NewMode);
    OnModeSwitched.Broadcast(NewMode);
}

void AGalleryGameModeBase::SwitchToWalk() { SwitchMode(EGalleryMode::Walk); }
void AGalleryGameModeBase::SwitchToTopView() { SwitchMode(EGalleryMode::TopView); }
void AGalleryGameModeBase::SwitchToPOI() { SwitchMode(EGalleryMode::POI); }

void AGalleryGameModeBase::TestTransition()
{
    if (CurrentPawn)
    {
        FVector TargetLocation = CurrentPawn->GetActorLocation() +
            CurrentPawn->GetActorForwardVector() * 300.0f;

        AActor* TestTarget = GetWorld()->SpawnActor<AActor>(
            AActor::StaticClass(), TargetLocation, FRotator::ZeroRotator);

        if (AGalleryPawnBase* GalleryPawn = Cast<AGalleryPawnBase>(CurrentPawn))
        {
            GalleryPawn->StartCameraTransitionTo(TestTarget, 2.0f);
            UE_LOG(LogTemp, Display, TEXT("[GameMode] Test transition started"));

            // Destroy test target after 3 seconds
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, [TestTarget]()
                {
                    if (TestTarget)
                    {
                        TestTarget->Destroy();
                        UE_LOG(LogTemp, Verbose, TEXT("[GameMode] Test target destroyed"));
                    }
                }, 3.0f, false);
        }
    }
}

void AGalleryGameModeBase::ChangePawnForMode(EGalleryMode Mode)
{
    UE_LOG(LogTemp, Display, TEXT("[GameMode] Changing pawn for mode: %s"),
        *UEnum::GetValueAsString(Mode));

    if (CurrentPawn)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[GameMode] Destroying current pawn: %s"),
            *CurrentPawn->GetName());
        CurrentPawn->Destroy();
        CurrentPawn = nullptr;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] No player controller found!"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Spawn location ABOVE ground to avoid getting stuck
    FVector SpawnLocation = FVector(0.f, 0.f, 300.f);  // 300 units above ground
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // Adjust spawn height based on mode
    switch (Mode)
    {
    case EGalleryMode::Walk:
        if (WalkPawnClass)
        {
            SpawnLocation.Z = 200.f;  // Lower for walk mode
            CurrentPawn = GetWorld()->SpawnActor<APawn>(WalkPawnClass,
                SpawnLocation, SpawnRotation, SpawnParams);
            UE_LOG(LogTemp, Display, TEXT("[GameMode] Spawned WalkCharacter at Z=200"));
        }
        break;

    case EGalleryMode::TopView:
        if (TopViewPawnClass)
        {
            SpawnLocation.Z = 1000.f;  // Higher for top view
            CurrentPawn = GetWorld()->SpawnActor<APawn>(TopViewPawnClass,
                SpawnLocation, SpawnRotation, SpawnParams);
            UE_LOG(LogTemp, Display, TEXT("[GameMode] Spawned TopViewPawn at Z=1000"));
        }
        break;

    case EGalleryMode::POI:
        if (POIPawnClass)
        {
            SpawnLocation.Z = 500.f;  // Mid height for POI
            CurrentPawn = GetWorld()->SpawnActor<APawn>(POIPawnClass,
                SpawnLocation, SpawnRotation, SpawnParams);
            UE_LOG(LogTemp, Display, TEXT("[GameMode] Spawned POIPawn at Z=500"));
        }
        break;
    }

    if (CurrentPawn)
    {
        PC->Possess(CurrentPawn);
        UE_LOG(LogTemp, Display, TEXT("[GameMode] Possessed new pawn: %s at %s"),
            *CurrentPawn->GetName(), *CurrentPawn->GetActorLocation().ToString());
    }
}