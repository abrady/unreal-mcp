#include "Commands/Animation/GetAnimSequenceMetadataCommand.h"
#include "Animation/AnimSequence.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetAnimSequenceMetadataCommand::FGetAnimSequenceMetadataCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FGetAnimSequenceMetadataCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SequencePath;
    if (!JsonObject->TryGetStringField(TEXT("sequence_path"), SequencePath) || SequencePath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Missing required 'sequence_path' parameter"));
    }

    UAnimSequence* Sequence = Service.FindAnimSequence(SequencePath);
    if (!Sequence)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Animation Sequence '%s' not found"), *SequencePath));
    }

    TSharedPtr<FJsonObject> MetadataObj;
    if (!Service.GetAnimSequenceMetadata(Sequence, MetadataObj))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve Animation Sequence metadata"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), MetadataObj);

    FString Out;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return Out;
}

FString FGetAnimSequenceMetadataCommand::GetCommandName() const
{
    return TEXT("get_anim_sequence_metadata");
}

bool FGetAnimSequenceMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    FString SequencePath;
    return JsonObject->TryGetStringField(TEXT("sequence_path"), SequencePath) && !SequencePath.IsEmpty();
}

FString FGetAnimSequenceMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString Out;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return Out;
}
