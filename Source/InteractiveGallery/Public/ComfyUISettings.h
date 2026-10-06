#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ComfyUISettings.generated.h"

// Project Settings > Game > ComfyUI (saved to Config/DefaultGame.ini)
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ComfyUI"))
class INTERACTIVEGALLERY_API UComfyUISettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return TEXT("Game"); }

    // Local ComfyUI server (start it with run_nvidia_gpu.bat)
    UPROPERTY(Config, EditAnywhere, Category = "Server")
    FString ServerURL = TEXT("http://127.0.0.1:8188");

    // API-format workflow, relative to the project folder. In ComfyUI: Workflow > Export (API).
    // Placeholders replaced before sending: "%PROMPT%" inside a text field, and the quoted
    // numbers "%SEED%", "%WIDTH%", "%HEIGHT%", "%STEPS%", "%CFG%".
    UPROPERTY(Config, EditAnywhere, Category = "Workflow")
    FString WorkflowFile = TEXT("ComfyUI/z_image_turbo_api.json");

    UPROPERTY(Config, EditAnywhere, Category = "Workflow", meta = (ClampMin = "1", ClampMax = "100"))
    int32 Steps = 8;

    UPROPERTY(Config, EditAnywhere, Category = "Workflow", meta = (ClampMin = "0"))
    float CFG = 1.0f;

    // Image size is picked to match each frame's aspect ratio at roughly this many megapixels
    UPROPERTY(Config, EditAnywhere, Category = "Workflow", meta = (ClampMin = "0.25", ClampMax = "4"))
    float Megapixels = 1.0f;

    // Added around every prompt, e.g. to keep a consistent gallery style
    UPROPERTY(Config, EditAnywhere, Category = "Prompt")
    FString PromptPrefix;

    UPROPERTY(Config, EditAnywhere, Category = "Prompt")
    FString PromptSuffix = TEXT(", fine art painting, visible brushwork, museum quality");

    UPROPERTY(Config, EditAnywhere, Category = "Server", meta = (ClampMin = "0.2"))
    float PollIntervalSeconds = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Server", meta = (ClampMin = "10"))
    float TimeoutSeconds = 300.f;
};
