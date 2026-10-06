#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "POITarget.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UMaterialVariantComponent;

// Place in the level to define a point of interest.
// The actor's location is the orbit pivot (put it on the object); the arrow is the view direction.
UCLASS()
class INTERACTIVEGALLERY_API APOITarget : public AActor
{
    GENERATED_BODY()

public:
    APOITarget();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI", meta = (MultiLine = true))
    FText Description;

    // POIs are visited in ascending order
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI")
    int32 Order = 0;

    // Camera distance from the pivot when arriving
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI|Camera", meta = (ClampMin = "10"))
    float ViewDistance = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI|Camera", meta = (ClampMin = "10"))
    float MinViewDistance = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI|Camera", meta = (ClampMin = "10"))
    float MaxViewDistance = 600.f;

    // Actors offered in the POI panel: their UMaterialVariantComponents get swatches,
    // and an AIPaintingFrame gets a prompt box
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI|Materials")
    TArray<TObjectPtr<AActor>> MaterialTargets;

    // Where the clickable Top view marker floats, relative to the pivot
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI|Marker")
    FVector MarkerOffset = FVector(0.f, 0.f, 80.f);

    UFUNCTION(BlueprintPure, Category = "POI|Marker")
    FVector GetMarkerLocation() const { return GetActorLocation() + MarkerOffset; }

    UFUNCTION(BlueprintPure, Category = "POI|Materials")
    TArray<UMaterialVariantComponent*> GetMaterialVariantComponents() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USceneComponent> Root;

#if WITH_EDITORONLY_DATA
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UArrowComponent> ViewArrow;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UBillboardComponent> Billboard;
#endif
};
