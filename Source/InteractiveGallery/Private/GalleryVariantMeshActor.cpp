#include "GalleryVariantMeshActor.h"
#include "MaterialVariantComponent.h"

AGalleryVariantMeshActor::AGalleryVariantMeshActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    MaterialVariants = CreateDefaultSubobject<UMaterialVariantComponent>(TEXT("MaterialVariants"));
}
