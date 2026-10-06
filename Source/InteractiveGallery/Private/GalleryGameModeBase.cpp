#include "GalleryGameModeBase.h"
#include "WalkCharacter.h"
#include "TopViewPawn.h"
#include "POIPawn.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

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

    // The engine has already spawned the starting pawn via SpawnDefaultPawnFor; just adopt it
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        CurrentPawn = PC->GetPawn();
    }

    if (!CurrentPawn)
    {
        ChangePawnForMode(CurrentMode);
    }
}

UClass* AGalleryGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    return GetPawnClassForMode(CurrentMode);
}

APawn* AGalleryGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    return SpawnPawnForMode(CurrentMode, NewPlayer);
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

    if (CurrentMode == EGalleryMode::Walk && CurrentPawn)
    {
        LastWalkTransform = CurrentPawn->GetActorTransform();
        bHasLastWalkTransform = true;
    }

    CurrentMode = NewMode;
    ChangePawnForMode(NewMode);
    OnModeSwitched.Broadcast(NewMode);
}

void AGalleryGameModeBase::CycleMode()
{
    switch (CurrentMode)
    {
    case EGalleryMode::Walk:    SwitchMode(EGalleryMode::TopView); break;
    case EGalleryMode::TopView: SwitchMode(EGalleryMode::POI);     break;
    case EGalleryMode::POI:     SwitchMode(EGalleryMode::Walk);    break;
    }
}

void AGalleryGameModeBase::SwitchToWalk() { SwitchMode(EGalleryMode::Walk); }
void AGalleryGameModeBase::SwitchToTopView() { SwitchMode(EGalleryMode::TopView); }
void AGalleryGameModeBase::SwitchToPOI() { SwitchMode(EGalleryMode::POI); }

void AGalleryGameModeBase::TestTransition()
{
    AGalleryPawnBase* GalleryPawn = Cast<AGalleryPawnBase>(CurrentPawn);
    if (!GalleryPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] TestTransition needs TopView or POI mode"));
        return;
    }

    // Move 300 units forward and turn 90 degrees
    FTransform Target = GalleryPawn->GetActorTransform();
    Target.AddToTranslation(GalleryPawn->GetActorForwardVector() * 300.0f);
    Target.ConcatenateRotation(FRotator(0.f, 90.f, 0.f).Quaternion());

    GalleryPawn->StartCameraTransitionToTransform(Target, 2.0f);
    UE_LOG(LogTemp, Display, TEXT("[GameMode] Test transition started"));
}

UClass* AGalleryGameModeBase::GetPawnClassForMode(EGalleryMode Mode) const
{
    switch (Mode)
    {
    case EGalleryMode::Walk:    return WalkPawnClass;
    case EGalleryMode::TopView: return TopViewPawnClass;
    case EGalleryMode::POI:     return POIPawnClass;
    }
    return nullptr;
}

FTransform AGalleryGameModeBase::GetSpawnTransformForMode(EGalleryMode Mode) const
{
    // 1. A designer-placed spawn point: any actor tagged "Spawn_Walk", "Spawn_TopView" or "Spawn_POI"
    const FName Tag(*(TEXT("Spawn_") + StaticEnum<EGalleryMode>()->GetNameStringByValue(static_cast<int64>(Mode))));
    TArray<AActor*> Tagged;
    UGameplayStatics::GetAllActorsWithTag(this, Tag, Tagged);
    if (Tagged.Num() > 0)
    {
        return Tagged[0]->GetActorTransform();
    }

    if (Mode == EGalleryMode::Walk && bHasLastWalkTransform)
    {
        return LastWalkTransform;
    }

    // 2. Offset from the first PlayerStart, or from the world origin if the level has none
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    bool bFoundPlayerStart = false;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        Location = It->GetActorLocation();
        Rotation.Yaw = It->GetActorRotation().Yaw;
        bFoundPlayerStart = true;
        break;
    }

    switch (Mode)
    {
    case EGalleryMode::Walk:    Location.Z += bFoundPlayerStart ? 0.f : 200.f; break;  // keep clear of the floor
    case EGalleryMode::TopView: Location.Z += 1000.f; break;
    case EGalleryMode::POI:     Location.Z += 500.f; break;
    }

    return FTransform(Rotation, Location);
}

APawn* AGalleryGameModeBase::SpawnPawnForMode(EGalleryMode Mode, AController* ForController)
{
    UClass* PawnClass = GetPawnClassForMode(Mode);
    if (!PawnClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] No pawn class set for %s"), *UEnum::GetValueAsString(Mode));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Instigator = ForController ? ForController->GetInstigator() : nullptr;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    const FTransform SpawnTransform = GetSpawnTransformForMode(Mode);
    APawn* NewPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnParams);

    UE_LOG(LogTemp, Display, TEXT("[GameMode] Spawned %s at %s"),
        NewPawn ? *NewPawn->GetName() : TEXT("nothing"), *SpawnTransform.GetLocation().ToString());
    return NewPawn;
}

void AGalleryGameModeBase::ChangePawnForMode(EGalleryMode Mode)
{
    UE_LOG(LogTemp, Display, TEXT("[GameMode] Changing pawn for mode: %s"),
        *UEnum::GetValueAsString(Mode));

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] No player controller found!"));
        return;
    }

    // Freeze the current view on a temporary camera, so we can blend from it after the old pawn is gone
    ACameraActor* BlendFromCamera = nullptr;
    if (CurrentPawn && ModeBlendTime > 0.f && PC->PlayerCameraManager)
    {
        const FMinimalViewInfo& View = PC->PlayerCameraManager->GetCameraCacheView();
        BlendFromCamera = GetWorld()->SpawnActor<ACameraActor>(View.Location, View.Rotation);
        if (BlendFromCamera)
        {
            BlendFromCamera->GetCameraComponent()->SetFieldOfView(View.FOV);
            BlendFromCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
            BlendFromCamera->SetLifeSpan(ModeBlendTime + 0.5f);
        }
    }

    if (CurrentPawn)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[GameMode] Destroying current pawn: %s"), *CurrentPawn->GetName());
        CurrentPawn->Destroy();
        CurrentPawn = nullptr;
    }

    CurrentPawn = SpawnPawnForMode(Mode, PC);
    if (!CurrentPawn)
    {
        return;
    }

    PC->Possess(CurrentPawn);

    if (BlendFromCamera)
    {
        PC->SetViewTarget(BlendFromCamera);
        PC->SetViewTargetWithBlend(CurrentPawn, ModeBlendTime, VTBlend_EaseInOut, 2.f);
    }

    UE_LOG(LogTemp, Display, TEXT("[GameMode] Possessed new pawn: %s"), *CurrentPawn->GetName());
}
