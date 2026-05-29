#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for retrieving structural metadata from a UAnimMontage:
 *   - Sections (name, next section, start/end time, length)
 *   - Slot animation tracks (slot name + per-segment anim references)
 *   - Notifies and NotifyStates (name, class, trigger time, duration, track index)
 * Read-only.
 */
class UNREALMCP_API FGetAnimMontageMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetAnimMontageMetadataCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
