#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UBehaviorTree;
class UBlackboardData;

/**
 * Read-only inspection of UBehaviorTree and UBlackboardData assets.
 * The BT tools were the largest blind spot in the existing MCP toolset —
 * neither asset class is a UBlueprint, so find_in_blueprints can't reach them.
 */
class UNREALMCP_API IBehaviorTreeService
{
public:
    virtual ~IBehaviorTreeService() = default;

    /** Find a UBehaviorTree by path or bare name. */
    virtual UBehaviorTree* FindBehaviorTree(const FString& Name) = 0;

    /**
     * Dump the BT's node tree as JSON: root composite recursing through children,
     * decorators, services, and tasks. Surfaces both native and Blueprint-derived
     * nodes; for BP nodes the source blueprint path is included so the caller can
     * follow up with get_blueprint_metadata.
     */
    virtual bool GetBehaviorTreeMetadata(UBehaviorTree* BehaviorTree, TSharedPtr<FJsonObject>& OutMetadata) = 0;

    /** Find a UBlackboardData by path or bare name. */
    virtual UBlackboardData* FindBlackboard(const FString& Name) = 0;

    /**
     * Dump blackboard keys with type tags. Walks the Parent chain and annotates
     * inherited entries.
     */
    virtual bool GetBlackboardMetadata(UBlackboardData* Blackboard, TSharedPtr<FJsonObject>& OutMetadata) = 0;
};
