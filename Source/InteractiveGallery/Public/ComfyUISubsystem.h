#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ComfyUISubsystem.generated.h"

class IHttpRequest;
class IHttpResponse;

// Sends text-to-image jobs to a local ComfyUI server:
//   POST /prompt (workflow with the prompt filled in) -> poll GET /history/<id> -> GET /view (the PNG)
UCLASS()
class INTERACTIVEGALLERY_API UComfyUISubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    using FOnProgress = TFunction<void(const FText& Status)>;
    using FOnComplete = TFunction<void(bool bSuccess, const TArray<uint8>& PngData, const FText& Error)>;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Callbacks run on the game thread. Width/Height are rounded to multiples of 64.
    void GenerateImage(const FString& Prompt, int32 Width, int32 Height, FOnProgress OnProgress, FOnComplete OnComplete);

    // Image size with the given aspect ratio at the configured megapixels
    static FIntPoint GetImageSizeForAspect(float Aspect);

private:
    struct FJob
    {
        FString PromptId;
        double StartTime = 0.0;
        FOnProgress OnProgress;
        FOnComplete OnComplete;
        FTimerHandle PollTimer;
    };

    FString ClientId;

    void PollHistory(TSharedRef<FJob> Job);
    void DownloadImage(TSharedRef<FJob> Job, const FString& Filename, const FString& Subfolder, const FString& Type);
    void Fail(TSharedRef<FJob> Job, const FText& Error);
    FText DescribeConnectionError() const;
    FString GetServerURL() const;

    static FString EscapeForJson(const FString& Text);
};
