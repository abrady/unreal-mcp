#pragma once

#include "CoreMinimal.h"
#include "Services/IBehaviorTreeService.h"

class UBTNode;
class UBTCompositeNode;
class UBTTaskNode;
class UBTDecorator;
class UBTService;

/**
 * Singleton implementation of IBehaviorTreeService.
 */
class UNREALMCP_API FBehaviorTreeService : public IBehaviorTreeService
{
public:
    static FBehaviorTreeService& Get();

    virtual UBehaviorTree* FindBehaviorTree(const FString& Name) override;
    virtual bool GetBehaviorTreeMetadata(UBehaviorTree* BehaviorTree, TSharedPtr<FJsonObject>& OutMetadata) override;

    virtual UBlackboardData* FindBlackboard(const FString& Name) override;
    virtual bool GetBlackboardMetadata(UBlackboardData* Blackboard, TSharedPtr<FJsonObject>& OutMetadata) override;

private:
    FBehaviorTreeService() = default;
    static TUniquePtr<FBehaviorTreeService> Instance;

    // ---- BT serialization helpers ----
    void SerializeNodeBase(UBTNode* Node, const TSharedPtr<FJsonObject>& OutObj) const;
    TSharedPtr<FJsonObject> SerializeDecorator(UBTDecorator* Decorator) const;
    TSharedPtr<FJsonObject> SerializeService(UBTService* Service) const;
    TSharedPtr<FJsonObject> SerializeTask(UBTTaskNode* Task) const;
    TSharedPtr<FJsonObject> SerializeComposite(UBTCompositeNode* Composite) const;
    TSharedPtr<FJsonObject> SerializeAnyNode(UBTNode* Node) const;
};
