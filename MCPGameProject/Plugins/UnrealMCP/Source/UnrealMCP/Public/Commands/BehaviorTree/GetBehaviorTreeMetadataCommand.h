#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBehaviorTreeService.h"

/**
 * Read-only command: dump a UBehaviorTree as JSON.
 * Walks the root composite recursively, surfacing decorators, services,
 * and tasks (including Blueprint-derived ones).
 */
class UNREALMCP_API FGetBehaviorTreeMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetBehaviorTreeMetadataCommand(IBehaviorTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBehaviorTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
