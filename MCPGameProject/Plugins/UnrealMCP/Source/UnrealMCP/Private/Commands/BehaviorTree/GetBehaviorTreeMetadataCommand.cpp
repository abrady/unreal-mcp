#include "Commands/BehaviorTree/GetBehaviorTreeMetadataCommand.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetBehaviorTreeMetadataCommand::FGetBehaviorTreeMetadataCommand(IBehaviorTreeService& InService)
    : Service(InService)
{
}

FString FGetBehaviorTreeMetadataCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString BTPath;
    if (!JsonObject->TryGetStringField(TEXT("bt_path"), BTPath) || BTPath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Missing required 'bt_path' parameter"));
    }

    UBehaviorTree* BehaviorTree = Service.FindBehaviorTree(BTPath);
    if (!BehaviorTree)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree '%s' not found"), *BTPath));
    }

    TSharedPtr<FJsonObject> MetadataObj;
    if (!Service.GetBehaviorTreeMetadata(BehaviorTree, MetadataObj))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve Behavior Tree metadata"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), MetadataObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetBehaviorTreeMetadataCommand::GetCommandName() const
{
    return TEXT("get_behavior_tree_metadata");
}

bool FGetBehaviorTreeMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString BTPath;
    return JsonObject->TryGetStringField(TEXT("bt_path"), BTPath) && !BTPath.IsEmpty();
}

FString FGetBehaviorTreeMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
