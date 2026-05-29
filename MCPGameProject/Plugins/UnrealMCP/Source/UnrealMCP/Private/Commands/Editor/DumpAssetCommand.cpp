#include "Commands/Editor/DumpAssetCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/World.h"
#include "Exporters/Exporter.h"
#include "UnrealExporter.h"
#include "Misc/StringOutputDevice.h"
#include "UObject/Package.h"

namespace
{
	// Hard cap on returned text so a UWorld-class export can't blow past the 48KB TCP buffer.
	constexpr int32 MaxDumpBytes = 262144; // 256KB

	FString MakeJsonResponse(const TSharedPtr<FJsonObject>& Obj)
	{
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Obj.ToSharedRef(), Writer);
		return Out;
	}

	FString MakeErrorJson(const FString& Msg)
	{
		// Local name (don't collide with UE's TValueOrError::MakeError).
		TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
		Err->SetBoolField(TEXT("success"), false);
		Err->SetStringField(TEXT("error"), Msg);
		return MakeJsonResponse(Err);
	}
}

FString FDumpAssetCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return MakeErrorJson(TEXT("Invalid JSON parameters"));
	}

	FString AssetPath;
	if (!JsonObject->TryGetStringField(TEXT("asset_path"), AssetPath) || AssetPath.IsEmpty())
	{
		return MakeErrorJson(TEXT("Missing required 'asset_path' parameter"));
	}

	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (!Asset)
	{
		return MakeErrorJson(FString::Printf(TEXT("Could not load asset at '%s'"), *AssetPath));
	}

	// UWorld exports are enormous (entire level T3D) — reject explicitly rather than truncating.
	if (Asset->IsA<UWorld>())
	{
		TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
		Err->SetBoolField(TEXT("success"), false);
		Err->SetStringField(TEXT("error"), TEXT("dump_asset does not support UWorld assets (use level-specific tools instead)"));
		Err->SetStringField(TEXT("asset_path"), AssetPath);
		Err->SetStringField(TEXT("class"), Asset->GetClass()->GetName());
		return MakeJsonResponse(Err);
	}

	FStringOutputDevice OutputDevice;
	const uint32 PortFlags = PPF_DeepCompareInstances | PPF_IncludeTransient;
	const bool bExported = UExporter::ExportToOutputDevice(
		nullptr,        // Context
		Asset,          // Object to export
		nullptr,        // Exporter (auto-pick)
		OutputDevice,
		TEXT("copy"),   // T3D text format
		0,              // Indent
		PortFlags,
		false,          // bSelectedOnly
		nullptr         // ExportRootScope
	);

	FString Dump = OutputDevice;
	const int32 OriginalLen = Dump.Len();
	bool bTruncated = false;
	if (OriginalLen > MaxDumpBytes)
	{
		Dump = Dump.Left(MaxDumpBytes);
		bTruncated = true;
	}

	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), bExported);
	ResponseObj->SetStringField(TEXT("asset_path"), AssetPath);
	ResponseObj->SetStringField(TEXT("class"), Asset->GetClass()->GetPathName());
	ResponseObj->SetStringField(TEXT("dump"), Dump);
	ResponseObj->SetNumberField(TEXT("length"), Dump.Len());
	ResponseObj->SetNumberField(TEXT("original_length"), OriginalLen);
	ResponseObj->SetBoolField(TEXT("truncated"), bTruncated);
	if (!bExported)
	{
		ResponseObj->SetStringField(TEXT("error"), TEXT("UExporter::ExportToOutputDevice returned false; partial dump may still be present"));
	}
	return MakeJsonResponse(ResponseObj);
}

FString FDumpAssetCommand::GetCommandName() const
{
	return TEXT("dump_asset");
}

bool FDumpAssetCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}
	FString AssetPath;
	return JsonObject->TryGetStringField(TEXT("asset_path"), AssetPath) && !AssetPath.IsEmpty();
}
