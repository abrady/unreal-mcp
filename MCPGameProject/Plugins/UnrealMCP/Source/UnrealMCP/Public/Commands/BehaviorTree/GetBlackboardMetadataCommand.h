#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBehaviorTreeService.h"

/**
 * Read-only command: dump a UBlackboardData asset's keys with type info,
 * walking the Parent chain.
 */
class UNREALMCP_API FGetBlackboardMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetBlackboardMetadataCommand(IBehaviorTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBehaviorTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
