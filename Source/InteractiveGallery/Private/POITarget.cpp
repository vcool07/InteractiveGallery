#include "POITarget.h"
#include "MaterialVariantComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"

APOITarget::APOITarget()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

#if WITH_EDITORONLY_DATA
    ViewArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ViewArrow"));
    if (ViewArrow)
    {
        ViewArrow->SetupAttachment(Root);
        ViewArrow->ArrowColor = FColor(255, 180, 40);
        ViewArrow->ArrowSize = 2.f;
    }

    Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    if (Billboard)
    {
        Billboard->SetupAttachment(Root);
    }
#endif
}

TArray<UMaterialVariantComponent*> APOITarget::GetMaterialVariantComponents() const
{
    TArray<UMaterialVariantComponent*> Result;
    for (AActor* Target : MaterialTargets)
    {
        if (Target)
        {
            TArray<UMaterialVariantComponent*> Components;
            Target->GetComponents<UMaterialVariantComponent>(Components);
            Result.Append(Components);
        }
    }
    return Result;
}
