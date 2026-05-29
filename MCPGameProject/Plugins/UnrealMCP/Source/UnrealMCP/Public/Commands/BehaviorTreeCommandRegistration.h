#pragma once

#include "CoreMinimal.h"

/**
 * Static class responsible for registering all BehaviorTree-domain commands
 * (BT and Blackboard metadata).
 */
class UNREALMCP_API FBehaviorTreeCommandRegistration
{
public:
    static void RegisterAllBehaviorTreeCommands();
    static void UnregisterAllBehaviorTreeCommands();

private:
    static TArray<FString> RegisteredCommandNames;

    static void RegisterGetBehaviorTreeMetadataCommand();
    static void RegisterGetBlackboardMetadataCommand();

    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};
