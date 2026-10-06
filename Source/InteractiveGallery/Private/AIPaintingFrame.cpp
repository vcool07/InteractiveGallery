#include "AIPaintingFrame.h"
#include "ComfyUISettings.h"
#include "ComfyUISubsystem.h"
#include "ImageUtils.h"
#include "MaterialVariantComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "AIPaintingFrame"

AAIPaintingFrame::AAIPaintingFrame()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(Root);
    FrameMesh->ComponentTags.Add(TEXT("Frame"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        FrameMesh->SetStaticMesh(CubeMesh.Object);
    }

    Canvas = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Canvas"));
    Canvas->SetupAttachment(Root);
    Canvas->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    FrameVariants = CreateDefaultSubobject<UMaterialVariantComponent>(TEXT("FrameVariants"));
    FrameVariants->TargetComponentTag = TEXT("Frame");
}

void AAIPaintingFrame::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    BuildCanvas();
}

void AAIPaintingFrame::BuildCanvas()
{
    // Frame: a box behind the canvas, FrameBorder larger on every side (BasicShapes cube is 100 cm)
    FrameMesh->SetRelativeScale3D(FVector(FrameDepth, CanvasWidth + 2.f * FrameBorder, CanvasHeight + 2.f * FrameBorder) / 100.f);
    if (FrameMaterial)
    {
        FrameMesh->SetMaterial(0, FrameMaterial);
    }

    // Canvas: a quad just in front of the frame, facing +X. Seen from the front, +Y is on the
    // viewer's left, so the image's top-left corner is at (+Y, +Z).
    const float X = FrameDepth * 0.5f + 0.2f;
    const float HalfW = CanvasWidth * 0.5f;
    const float HalfH = CanvasHeight * 0.5f;

    const TArray<FVector> Vertices = {
        FVector(X,  HalfW,  HalfH),  // top-left
        FVector(X, -HalfW,  HalfH),  // top-right
        FVector(X,  HalfW, -HalfH),  // bottom-left
        FVector(X, -HalfW, -HalfH),  // bottom-right
    };
    const TArray<int32> Triangles = { 0, 2, 1, 1, 2, 3 };
    const TArray<FVector> Normals = { FVector::ForwardVector, FVector::ForwardVector, FVector::ForwardVector, FVector::ForwardVector };
    const TArray<FVector2D> UVs = { FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1), FVector2D(1, 1) };
    const TArray<FProcMeshTangent> Tangents = { FProcMeshTangent(0, -1, 0), FProcMeshTangent(0, -1, 0), FProcMeshTangent(0, -1, 0), FProcMeshTangent(0, -1, 0) };

    Canvas->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), Tangents, false);
    if (CanvasMaterial)
    {
        Canvas->SetMaterial(0, CanvasMaterial);
    }
}

void AAIPaintingFrame::BeginPlay()
{
    Super::BeginPlay();

    if (CanvasMaterial)
    {
        CanvasMID = Canvas->CreateDynamicMaterialInstance(0, CanvasMaterial);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIPaintingFrame] %s has no CanvasMaterial - paintings won't show"), *GetName());
    }

    // Show the last painting from a previous session
    LastPrompt = DefaultPrompt;
    const FString BasePath = GetSaveBasePath();
    if (FPaths::FileExists(BasePath + TEXT("_latest.png")))
    {
        ShowArtwork(FImageUtils::ImportFileAsTexture2D(BasePath + TEXT("_latest.png")));
        FFileHelper::LoadFileToString(LastPrompt, *(BasePath + TEXT("_latest.txt")));
    }

    SetStatus(EPaintingStatus::Idle, LOCTEXT("Ready", "Describe a painting and press Paint."));
}

FString AAIPaintingFrame::GetSaveBasePath() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GeneratedArt"), GetName());
}

void AAIPaintingFrame::Paint(const FString& Prompt)
{
    if (Status == EPaintingStatus::Working)
    {
        return;
    }

    const FString CleanPrompt = Prompt.TrimStartAndEnd();
    if (CleanPrompt.IsEmpty())
    {
        SetStatus(EPaintingStatus::Failed, LOCTEXT("EmptyPrompt", "Type a prompt first."));
        return;
    }

    UComfyUISubsystem* Comfy = GetGameInstance() ? GetGameInstance()->GetSubsystem<UComfyUISubsystem>() : nullptr;
    if (!Comfy)
    {
        return;
    }

    LastPrompt = CleanPrompt;
    PaintStartTime = FPlatformTime::Seconds();
    SetStatus(EPaintingStatus::Working, LOCTEXT("Starting", "Starting..."));

    const FIntPoint Size = UComfyUISubsystem::GetImageSizeForAspect(CanvasWidth / CanvasHeight);
    TWeakObjectPtr<AAIPaintingFrame> WeakThis(this);
    Comfy->GenerateImage(CleanPrompt, Size.X, Size.Y,
        [WeakThis](const FText& Progress)
        {
            if (WeakThis.IsValid())
            {
                WeakThis->SetStatus(EPaintingStatus::Working, Progress);
            }
        },
        [WeakThis](bool bSuccess, const TArray<uint8>& PngData, const FText& Error)
        {
            if (WeakThis.IsValid())
            {
                WeakThis->HandleImage(bSuccess, PngData, Error);
            }
        });
}

void AAIPaintingFrame::HandleImage(bool bSuccess, const TArray<uint8>& PngData, const FText& Error)
{
    if (!bSuccess)
    {
        SetStatus(EPaintingStatus::Failed, Error);
        return;
    }

    UTexture2D* Texture = FImageUtils::ImportBufferAsTexture2D(PngData);
    if (!Texture)
    {
        SetStatus(EPaintingStatus::Failed, LOCTEXT("BadImage", "ComfyUI sent an image Unreal couldn't read."));
        return;
    }
    ShowArtwork(Texture);

    // Keep every painting, and remember the latest for next session
    const FString BasePath = GetSaveBasePath();
    const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    FFileHelper::SaveArrayToFile(PngData, *(BasePath + TEXT("_") + Stamp + TEXT(".png")));
    FFileHelper::SaveArrayToFile(PngData, *(BasePath + TEXT("_latest.png")));
    FFileHelper::SaveStringToFile(LastPrompt, *(BasePath + TEXT("_latest.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const int32 Seconds = FMath::RoundToInt(FPlatformTime::Seconds() - PaintStartTime);
    SetStatus(EPaintingStatus::Done, FText::Format(LOCTEXT("Done", "Painted in {0}s. Saved to Saved/GeneratedArt."), Seconds));
}

void AAIPaintingFrame::ShowArtwork(UTexture2D* Texture)
{
    if (Texture)
    {
        Artwork = Texture;
        if (CanvasMID)
        {
            CanvasMID->SetTextureParameterValue(ArtworkParameter, Texture);
        }
    }
}

void AAIPaintingFrame::SetStatus(EPaintingStatus NewStatus, const FText& Text)
{
    Status = NewStatus;
    StatusText = Text;
    OnStatusChanged.Broadcast(this, NewStatus);
}

#undef LOCTEXT_NAMESPACE
