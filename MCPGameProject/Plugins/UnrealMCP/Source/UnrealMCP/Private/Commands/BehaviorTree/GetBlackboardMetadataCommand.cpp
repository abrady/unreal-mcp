#include "Commands/BehaviorTree/GetBlackboardMetadataCommand.h"
#include "BehaviorTree/BlackboardData.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetBlackboardMetadataCommand::FGetBlackboardMetadataCommand(IBehaviorTreeService& InService)
    : Service(InService)
{
}

FString FGetBlackboardMetadataCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString BBPath;
    if (!JsonObject->TryGetStringField(TEXT("blackboard_path"), BBPath) || BBPath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Missing required 'blackboard_path' parameter"));
    }

    UBlackboardData* Blackboard = Service.FindBlackboard(BBPath);
    if (!Blackboard)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blackboard '%s' not found"), *BBPath));
    }

    TSharedPtr<FJsonObject> MetadataObj;
    if (!Service.GetBlackboardMetadata(Blackboard, MetadataObj))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve Blackboard metadata"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), MetadataObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetBlackboardMetadataCommand::GetCommandName() const
{
    return TEXT("get_blackboard_metadata");
}

bool FGetBlackboardMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString BBPath;
    return JsonObject->TryGetStringField(TEXT("blackboard_path"), BBPath) && !BBPath.IsEmpty();
}

FString FGetBlackboardMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
