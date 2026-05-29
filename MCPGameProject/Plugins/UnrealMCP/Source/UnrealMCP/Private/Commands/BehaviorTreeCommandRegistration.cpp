#include "Commands/BehaviorTreeCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/BehaviorTree/GetBehaviorTreeMetadataCommand.h"
#include "Commands/BehaviorTree/GetBlackboardMetadataCommand.h"
#include "Services/BehaviorTreeService.h"

TArray<FString> FBehaviorTreeCommandRegistration::RegisteredCommandNames;

void FBehaviorTreeCommandRegistration::RegisterAllBehaviorTreeCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FBehaviorTreeCommandRegistration::RegisterAllBehaviorTreeCommands: Starting"));
    RegisteredCommandNames.Empty();

    RegisterGetBehaviorTreeMetadataCommand();
    RegisterGetBlackboardMetadataCommand();

    UE_LOG(LogTemp, Log, TEXT("FBehaviorTreeCommandRegistration::RegisterAllBehaviorTreeCommands: Registered %d commands"),
        RegisteredCommandNames.Num());
}

void FBehaviorTreeCommandRegistration::UnregisterAllBehaviorTreeCommands()
{
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    int32 UnregisteredCount = 0;
    for (const FString& CommandName : RegisteredCommandNames)
    {
        if (Registry.UnregisterCommand(CommandName))
        {
            ++UnregisteredCount;
        }
    }
    RegisteredCommandNames.Empty();
    UE_LOG(LogTemp, Log, TEXT("FBehaviorTreeCommandRegistration::UnregisterAllBehaviorTreeCommands: Unregistered %d commands"),
        UnregisteredCount);
}

void FBehaviorTreeCommandRegistration::RegisterGetBehaviorTreeMetadataCommand()
{
    TSharedPtr<FGetBehaviorTreeMetadataCommand> Command = MakeShared<FGetBehaviorTreeMetadataCommand>(FBehaviorTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBehaviorTreeCommandRegistration::RegisterGetBlackboardMetadataCommand()
{
    TSharedPtr<FGetBlackboardMetadataCommand> Command = MakeShared<FGetBlackboardMetadataCommand>(FBehaviorTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBehaviorTreeCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FBehaviorTreeCommandRegistration::RegisterAndTrackCommand: Invalid command"));
        return;
    }

    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FBehaviorTreeCommandRegistration::RegisterAndTrackCommand: Command has empty name"));
        return;
    }

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommandNames.Add(CommandName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FBehaviorTreeCommandRegistration::RegisterAndTrackCommand: Failed to register command '%s'"), *CommandName);
    }
}
