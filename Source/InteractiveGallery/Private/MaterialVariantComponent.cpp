#include "MaterialVariantComponent.h"
#include "Components/MeshComponent.h"

UMaterialVariantComponent::UMaterialVariantComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMaterialVariantComponent::BeginPlay()
{
    Super::BeginPlay();

    if (Variants.IsValidIndex(InitialVariant))
    {
        ApplyVariant(InitialVariant);
    }
}

void UMaterialVariantComponent::ApplyVariant(int32 Index)
{
    if (!Variants.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Warning, TEXT("[MaterialVariant] %s: no variant %d"), *GetNameSafe(GetOwner()), Index);
        return;
    }

    for (UMeshComponent* Mesh : GetTargetMeshes())
    {
        Mesh->SetMaterial(MaterialSlotIndex, Variants[Index].Material);
    }

    CurrentVariant = Index;
    OnVariantChanged.Broadcast(this, Index);
    UE_LOG(LogTemp, Display, TEXT("[MaterialVariant] %s -> %s"),
        *GetNameSafe(GetOwner()), *Variants[Index].DisplayName.ToString());
}

TArray<UMeshComponent*> UMaterialVariantComponent::GetTargetMeshes() const
{
    TArray<UMeshComponent*> Meshes;
    TArray<AActor*> Actors(LinkedActors);
    Actors.Add(GetOwner());

    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            TArray<UMeshComponent*> ActorMeshes;
            Actor->GetComponents<UMeshComponent>(ActorMeshes);
            Meshes.Append(ActorMeshes);
        }
    }

    if (!TargetComponentTag.IsNone())
    {
        Meshes.RemoveAll([this](const UMeshComponent* Mesh) { return !Mesh->ComponentHasTag(TargetComponentTag); });
    }
    return Meshes;
}
