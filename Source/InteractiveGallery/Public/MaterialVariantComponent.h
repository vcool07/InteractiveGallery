#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaterialVariantComponent.generated.h"

class UMaterialInterface;
class UMaterialVariantComponent;
class UMeshComponent;
class UTexture2D;

USTRUCT(BlueprintType)
struct FMaterialVariant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
    TObjectPtr<UMaterialInterface> Material = nullptr;

    // Shown on the UI button; if empty, the button shows SwatchColor instead
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
    TObjectPtr<UTexture2D> Thumbnail = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
    FLinearColor SwatchColor = FLinearColor::Gray;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMaterialVariantChangedSignature, UMaterialVariantComponent*, Component, int32, NewIndex);

// Add to any actor with a mesh to make one of its material slots switchable (e.g. from a POI panel)
UCLASS(ClassGroup = (Gallery), meta = (BlueprintSpawnableComponent))
class INTERACTIVEGALLERY_API UMaterialVariantComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMaterialVariantComponent();

    // Heading shown above this component's swatches, e.g. "Wall finish"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variants")
    FText SlotLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variants")
    TArray<FMaterialVariant> Variants;

    // Material slot on the target mesh(es) that gets replaced
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variants")
    int32 MaterialSlotIndex = 0;

    // Only meshes with this component tag are affected; if None, every mesh on the owner is
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variants")
    FName TargetComponentTag = NAME_None;

    // Other actors that change together with this one (e.g. every wall of a room)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variants")
    TArray<TObjectPtr<AActor>> LinkedActors;

    // Variant applied on BeginPlay (-1 keeps the mesh's own material)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variants")
    int32 InitialVariant = INDEX_NONE;

    UPROPERTY(BlueprintAssignable, Category = "Variants")
    FOnMaterialVariantChangedSignature OnVariantChanged;

    UFUNCTION(BlueprintCallable, Category = "Variants")
    void ApplyVariant(int32 Index);

    UFUNCTION(BlueprintPure, Category = "Variants")
    int32 GetCurrentVariant() const { return CurrentVariant; }

protected:
    virtual void BeginPlay() override;

private:
    int32 CurrentVariant = INDEX_NONE;

    TArray<UMeshComponent*> GetTargetMeshes() const;
};
