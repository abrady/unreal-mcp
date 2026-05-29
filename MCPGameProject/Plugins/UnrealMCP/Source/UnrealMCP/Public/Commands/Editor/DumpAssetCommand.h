#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Universal asset-dump command. Loads any UObject by package path and serializes it
 * via UExporter::ExportToOutputDevice in "copy" format (Unreal T3D text). Returns the
 * raw text. Useful as a fallback when a typed metadata tool does not yet exist for an
 * asset class (e.g. UAnimMontage section/notify tracks, UBehaviorTree node graphs).
 */
class UNREALMCP_API FDumpAssetCommand : public IUnrealMCPCommand
{
public:
	FDumpAssetCommand() = default;

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
