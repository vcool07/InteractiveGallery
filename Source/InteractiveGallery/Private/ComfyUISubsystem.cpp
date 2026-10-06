#include "ComfyUISubsystem.h"
#include "ComfyUISettings.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ComfyUI"

namespace
{
    TSharedPtr<FJsonObject> ParseJsonObject(const FString& Text)
    {
        TSharedPtr<FJsonObject> Object;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object);
        return Object;
    }

    bool IsOk(FHttpResponsePtr Response, bool bConnected)
    {
        return bConnected && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
    }
}

void UComfyUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ClientId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
}

FString UComfyUISubsystem::GetServerURL() const
{
    FString URL = GetDefault<UComfyUISettings>()->ServerURL;
    URL.RemoveFromEnd(TEXT("/"));
    return URL;
}

FText UComfyUISubsystem::DescribeConnectionError() const
{
    return FText::Format(LOCTEXT("NoServer", "Can't reach ComfyUI at {0}. Start it with run_nvidia_gpu.bat."),
        FText::FromString(GetServerURL()));
}

FIntPoint UComfyUISubsystem::GetImageSizeForAspect(float Aspect)
{
    const float Pixels = GetDefault<UComfyUISettings>()->Megapixels * 1024.f * 1024.f;
    const float SafeAspect = FMath::Clamp(Aspect, 0.25f, 4.f);
    auto RoundTo64 = [](float Value) { return FMath::Max(256, FMath::RoundToInt(Value / 64.f) * 64); };
    return FIntPoint(RoundTo64(FMath::Sqrt(Pixels * SafeAspect)), RoundTo64(FMath::Sqrt(Pixels / SafeAspect)));
}

FString UComfyUISubsystem::EscapeForJson(const FString& Text)
{
    return Text.Replace(TEXT("\\"), TEXT("\\\\"))
        .Replace(TEXT("\""), TEXT("\\\""))
        .Replace(TEXT("\r"), TEXT(""))
        .Replace(TEXT("\n"), TEXT("\\n"))
        .Replace(TEXT("\t"), TEXT(" "));
}

void UComfyUISubsystem::GenerateImage(const FString& Prompt, int32 Width, int32 Height, FOnProgress OnProgress, FOnComplete OnComplete)
{
    TSharedRef<FJob> Job = MakeShared<FJob>();
    Job->OnProgress = MoveTemp(OnProgress);
    Job->OnComplete = MoveTemp(OnComplete);
    Job->StartTime = FPlatformTime::Seconds();

    const UComfyUISettings* Settings = GetDefault<UComfyUISettings>();

    // 1. Fill the workflow template
    const FString WorkflowPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Settings->WorkflowFile);
    FString Workflow;
    if (!FFileHelper::LoadFileToString(Workflow, *WorkflowPath))
    {
        Fail(Job, FText::Format(LOCTEXT("NoWorkflow", "Workflow file not found: {0}"), FText::FromString(WorkflowPath)));
        return;
    }

    const int64 Seed = (static_cast<int64>(FMath::Rand()) << 16) ^ FMath::Rand();
    Workflow.ReplaceInline(TEXT("%PROMPT%"), *EscapeForJson(Settings->PromptPrefix + Prompt + Settings->PromptSuffix));
    Workflow.ReplaceInline(TEXT("\"%SEED%\""), *LexToString(Seed));
    Workflow.ReplaceInline(TEXT("\"%WIDTH%\""), *LexToString(FMath::Max(64, Width / 64 * 64)));
    Workflow.ReplaceInline(TEXT("\"%HEIGHT%\""), *LexToString(FMath::Max(64, Height / 64 * 64)));
    Workflow.ReplaceInline(TEXT("\"%STEPS%\""), *LexToString(Settings->Steps));
    Workflow.ReplaceInline(TEXT("\"%CFG%\""), *FString::SanitizeFloat(Settings->CFG));

    TSharedPtr<FJsonObject> WorkflowObject = ParseJsonObject(Workflow);
    if (!WorkflowObject.IsValid())
    {
        Fail(Job, FText::Format(LOCTEXT("BadWorkflow", "Workflow isn't valid JSON after filling placeholders: {0}"), FText::FromString(WorkflowPath)));
        return;
    }

    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetObjectField(TEXT("prompt"), WorkflowObject);
    Body->SetStringField(TEXT("client_id"), ClientId);
    FString BodyText;
    FJsonSerializer::Serialize(Body, TJsonWriterFactory<>::Create(&BodyText));

    // 2. Queue it
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GetServerURL() + TEXT("/prompt"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(BodyText);
    Request->OnProcessRequestComplete().BindWeakLambda(this, [this, Job](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
        {
            if (!bConnected || !Response.IsValid())
            {
                Fail(Job, DescribeConnectionError());
                return;
            }

            const TSharedPtr<FJsonObject> Result = ParseJsonObject(Response->GetContentAsString());
            if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()) || !Result.IsValid() || !Result->TryGetStringField(TEXT("prompt_id"), Job->PromptId))
            {
                // ComfyUI explains rejected workflows in error.message / error.details (e.g. a missing model)
                FString Message = Response->GetContentAsString().Left(300);
                const TSharedPtr<FJsonObject>* Error = nullptr;
                if (Result.IsValid() && Result->TryGetObjectField(TEXT("error"), Error))
                {
                    Message = (*Error)->GetStringField(TEXT("message")) + TEXT(" ") + (*Error)->GetStringField(TEXT("details"));
                }
                Fail(Job, FText::Format(LOCTEXT("Rejected", "ComfyUI rejected the workflow: {0}"), FText::FromString(Message)));
                return;
            }

            UE_LOG(LogTemp, Display, TEXT("[ComfyUI] Queued prompt %s"), *Job->PromptId);
            if (Job->OnProgress)
            {
                Job->OnProgress(LOCTEXT("Queued", "Queued in ComfyUI..."));
            }
            PollHistory(Job);
        });

    if (Job->OnProgress)
    {
        Job->OnProgress(LOCTEXT("Sending", "Sending to ComfyUI..."));
    }
    Request->ProcessRequest();
}

void UComfyUISubsystem::PollHistory(TSharedRef<FJob> Job)
{
    const UComfyUISettings* Settings = GetDefault<UComfyUISettings>();
    const double Elapsed = FPlatformTime::Seconds() - Job->StartTime;
    if (Elapsed > Settings->TimeoutSeconds)
    {
        Fail(Job, FText::Format(LOCTEXT("Timeout", "ComfyUI took longer than {0} seconds."), FMath::RoundToInt(Settings->TimeoutSeconds)));
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GetServerURL() + TEXT("/history/") + Job->PromptId);
    Request->SetVerb(TEXT("GET"));
    Request->OnProcessRequestComplete().BindWeakLambda(this, [this, Job](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
        {
            if (!IsOk(Response, bConnected))
            {
                Fail(Job, DescribeConnectionError());
                return;
            }

            // History is {} until the prompt finishes, then { "<id>": { "outputs": {...}, "status": {...} } }
            const TSharedPtr<FJsonObject> History = ParseJsonObject(Response->GetContentAsString());
            const TSharedPtr<FJsonObject>* Entry = nullptr;
            if (History.IsValid() && History->TryGetObjectField(Job->PromptId, Entry))
            {
                const TSharedPtr<FJsonObject>* Status = nullptr;
                if ((*Entry)->TryGetObjectField(TEXT("status"), Status) && (*Status)->GetStringField(TEXT("status_str")) == TEXT("error"))
                {
                    FString Message = TEXT("execution error");
                    const TArray<TSharedPtr<FJsonValue>>* Messages = nullptr;
                    if ((*Status)->TryGetArrayField(TEXT("messages"), Messages))
                    {
                        for (const TSharedPtr<FJsonValue>& Item : *Messages)
                        {
                            const TArray<TSharedPtr<FJsonValue>>& Pair = Item->AsArray();
                            if (Pair.Num() == 2 && Pair[0]->AsString() == TEXT("execution_error"))
                            {
                                Pair[1]->AsObject()->TryGetStringField(TEXT("exception_message"), Message);
                            }
                        }
                    }
                    Fail(Job, FText::Format(LOCTEXT("ExecError", "ComfyUI failed: {0}"), FText::FromString(Message.TrimStartAndEnd())));
                    return;
                }

                const TSharedPtr<FJsonObject>* Outputs = nullptr;
                if ((*Entry)->TryGetObjectField(TEXT("outputs"), Outputs))
                {
                    for (const TPair<FString, TSharedPtr<FJsonValue>>& Node : (*Outputs)->Values)
                    {
                        const TArray<TSharedPtr<FJsonValue>>* Images = nullptr;
                        if (Node.Value->AsObject()->TryGetArrayField(TEXT("images"), Images) && Images->Num() > 0)
                        {
                            const TSharedPtr<FJsonObject> Image = (*Images)[0]->AsObject();
                            DownloadImage(Job, Image->GetStringField(TEXT("filename")),
                                Image->GetStringField(TEXT("subfolder")), Image->GetStringField(TEXT("type")));
                            return;
                        }
                    }
                }
            }

            if (Job->OnProgress)
            {
                const int32 Seconds = FMath::RoundToInt(FPlatformTime::Seconds() - Job->StartTime);
                Job->OnProgress(FText::Format(LOCTEXT("Painting", "Painting... {0}s"), Seconds));
            }

            if (UGameInstance* GameInstance = GetGameInstance())
            {
                GameInstance->GetTimerManager().SetTimer(Job->PollTimer,
                    FTimerDelegate::CreateWeakLambda(this, [this, Job]() { PollHistory(Job); }),
                    GetDefault<UComfyUISettings>()->PollIntervalSeconds, false);
            }
        });
    Request->ProcessRequest();
}

void UComfyUISubsystem::DownloadImage(TSharedRef<FJob> Job, const FString& Filename, const FString& Subfolder, const FString& Type)
{
    if (Job->OnProgress)
    {
        Job->OnProgress(LOCTEXT("Downloading", "Hanging the painting..."));
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("%s/view?filename=%s&subfolder=%s&type=%s"), *GetServerURL(),
        *FGenericPlatformHttp::UrlEncode(Filename), *FGenericPlatformHttp::UrlEncode(Subfolder), *FGenericPlatformHttp::UrlEncode(Type)));
    Request->SetVerb(TEXT("GET"));
    Request->OnProcessRequestComplete().BindWeakLambda(this, [this, Job, Filename](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
        {
            if (!IsOk(Response, bConnected) || Response->GetContent().Num() == 0)
            {
                Fail(Job, LOCTEXT("DownloadFailed", "Couldn't download the image from ComfyUI."));
                return;
            }

            UE_LOG(LogTemp, Display, TEXT("[ComfyUI] Got %s (%d bytes) after %.1fs"),
                *Filename, Response->GetContent().Num(), FPlatformTime::Seconds() - Job->StartTime);
            if (Job->OnComplete)
            {
                Job->OnComplete(true, Response->GetContent(), FText::GetEmpty());
            }
        });
    Request->ProcessRequest();
}

void UComfyUISubsystem::Fail(TSharedRef<FJob> Job, const FText& Error)
{
    UE_LOG(LogTemp, Warning, TEXT("[ComfyUI] %s"), *Error.ToString());
    if (Job->OnComplete)
    {
        Job->OnComplete(false, TArray<uint8>(), Error);
    }
}

#undef LOCTEXT_NAMESPACE
