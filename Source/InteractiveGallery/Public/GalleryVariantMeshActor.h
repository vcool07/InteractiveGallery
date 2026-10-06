#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GalleryVariantMeshActor.generated.h"

class UMaterialVariantComponent;

// Convenience actor: a static mesh with a UMaterialVariantComponent already attached
UCLASS()
class INTERACTIVEGALLERY_API AGalleryVariantMeshActor : public AActor
{
    GENERATED_BODY()

public:
    AGalleryVariantMeshActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UMaterialVariantComponent> MaterialVariants;
};
