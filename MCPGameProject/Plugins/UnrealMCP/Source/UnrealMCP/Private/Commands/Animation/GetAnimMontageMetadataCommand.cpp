#include "Commands/Animation/GetAnimMontageMetadataCommand.h"
#include "Animation/AnimMontage.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetAnimMontageMetadataCommand::FGetAnimMontageMetadataCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FGetAnimMontageMetadataCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString MontagePath;
    if (!JsonObject->TryGetStringField(TEXT("montage_path"), MontagePath) || MontagePath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Missing required 'montage_path' parameter"));
    }

    UAnimMontage* Montage = Service.FindAnimMontage(MontagePath);
    if (!Montage)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Animation Montage '%s' not found"), *MontagePath));
    }

    TSharedPtr<FJsonObject> MetadataObj;
    if (!Service.GetAnimMontageMetadata(Montage, MetadataObj))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve Animation Montage metadata"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), MetadataObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetAnimMontageMetadataCommand::GetCommandName() const
{
    return TEXT("get_anim_montage_metadata");
}

bool FGetAnimMontageMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MontagePath;
    return JsonObject->TryGetStringField(TEXT("montage_path"), MontagePath) && !MontagePath.IsEmpty();
}

FString FGetAnimMontageMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
