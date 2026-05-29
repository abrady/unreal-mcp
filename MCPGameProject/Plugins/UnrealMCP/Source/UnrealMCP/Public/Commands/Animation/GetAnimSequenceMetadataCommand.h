#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Read-only: dump a UAnimSequence's structural metadata (length, skeleton,
 * notifies, sync markers). Useful for inspecting raw animation clips and
 * confirming the hit-window AnimNotifies authored on them.
 */
class UNREALMCP_API FGetAnimSequenceMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetAnimSequenceMetadataCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
