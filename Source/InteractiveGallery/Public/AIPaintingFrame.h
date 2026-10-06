#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIPaintingFrame.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialVariantComponent;
class UProceduralMeshComponent;
class UTexture2D;

UENUM(BlueprintType)
enum class EPaintingStatus : uint8
{
    Idle,
    Working,
    Done,
    Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPaintingStatusChangedSignature, AAIPaintingFrame*, Frame, EPaintingStatus, Status);

// A framed canvas that paints itself from a text prompt via ComfyUI.
// Faces +X (the arrow): place it with its back against a wall. Paintings are saved to
// Saved/GeneratedArt and the latest one is shown again next time.
UCLASS()
class INTERACTIVEGALLERY_API AAIPaintingFrame : public AActor
{
    GENERATED_BODY()

public:
    AAIPaintingFrame();

    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting", meta = (MultiLine = true))
    FString DefaultPrompt = TEXT("a seashell glowing on a moonlit beach");

    // Visible canvas size in cm (the generated image matches its aspect ratio)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting|Size", meta = (ClampMin = "10"))
    float CanvasWidth = 160.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting|Size", meta = (ClampMin = "10"))
    float CanvasHeight = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting|Size", meta = (ClampMin = "0"))
    float FrameBorder = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting|Size", meta = (ClampMin = "1"))
    float FrameDepth = 6.f;

    // Needs a texture parameter named ArtworkParameter
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting|Look")
    TObjectPtr<UMaterialInterface> CanvasMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting|Look")
    FName ArtworkParameter = TEXT("Artwork");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Painting|Look")
    TObjectPtr<UMaterialInterface> FrameMaterial;

    UPROPERTY(BlueprintAssignable, Category = "Painting")
    FOnPaintingStatusChangedSignature OnStatusChanged;

    UFUNCTION(BlueprintCallable, Category = "Painting")
    void Paint(const FString& Prompt);

    UFUNCTION(BlueprintPure, Category = "Painting")
    EPaintingStatus GetStatus() const { return Status; }

    UFUNCTION(BlueprintPure, Category = "Painting")
    FText GetStatusText() const { return StatusText; }

    UFUNCTION(BlueprintPure, Category = "Painting")
    FString GetLastPrompt() const { return LastPrompt; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UStaticMeshComponent> FrameMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UProceduralMeshComponent> Canvas;

    // Frame finishes offered in the POI panel (affects only the frame, via its "Frame" tag)
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UMaterialVariantComponent> FrameVariants;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> CanvasMID;

    UPROPERTY()
    TObjectPtr<UTexture2D> Artwork;

    EPaintingStatus Status = EPaintingStatus::Idle;
    FText StatusText;
    FString LastPrompt;
    double PaintStartTime = 0.0;

    void BuildCanvas();
    void ShowArtwork(UTexture2D* Texture);
    void SetStatus(EPaintingStatus NewStatus, const FText& Text);
    void HandleImage(bool bSuccess, const TArray<uint8>& PngData, const FText& Error);

    // Saved/GeneratedArt/<actor name>
    FString GetSaveBasePath() const;
};
