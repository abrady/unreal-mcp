#include "Services/BehaviorTreeService.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_NativeEnum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Struct.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectIterator.h"

TUniquePtr<FBehaviorTreeService> FBehaviorTreeService::Instance;

FBehaviorTreeService& FBehaviorTreeService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = TUniquePtr<FBehaviorTreeService>(new FBehaviorTreeService());
    }
    return *Instance;
}

namespace
{
    template <typename TAssetClass>
    TAssetClass* FindAssetByPathOrName(const FString& Name)
    {
        if (Name.IsEmpty())
        {
            return nullptr;
        }

        if (Name.StartsWith(TEXT("/Game/")) || Name.StartsWith(TEXT("/Script/")))
        {
            return LoadObject<TAssetClass>(nullptr, *Name);
        }

        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        TArray<FAssetData> AssetDataList;
        AssetRegistry.GetAssetsByClass(TAssetClass::StaticClass()->GetClassPathName(), AssetDataList);
        for (const FAssetData& AssetData : AssetDataList)
        {
            if (AssetData.AssetName.ToString() == Name)
            {
                return Cast<TAssetClass>(AssetData.GetAsset());
            }
        }
        return nullptr;
    }

    FString BlackboardKeyTypeName(UBlackboardKeyType* KeyType)
    {
        if (!KeyType)
        {
            return TEXT("unknown");
        }
        if (KeyType->IsA<UBlackboardKeyType_Bool>())       return TEXT("bool");
        if (KeyType->IsA<UBlackboardKeyType_Int>())        return TEXT("int");
        if (KeyType->IsA<UBlackboardKeyType_Float>())      return TEXT("float");
        if (KeyType->IsA<UBlackboardKeyType_String>())     return TEXT("string");
        if (KeyType->IsA<UBlackboardKeyType_Name>())       return TEXT("name");
        if (KeyType->IsA<UBlackboardKeyType_Vector>())     return TEXT("vector");
        if (KeyType->IsA<UBlackboardKeyType_Rotator>())    return TEXT("rotator");
        if (KeyType->IsA<UBlackboardKeyType_Object>())     return TEXT("object");
        if (KeyType->IsA<UBlackboardKeyType_Class>())      return TEXT("class");
        if (KeyType->IsA<UBlackboardKeyType_Enum>())       return TEXT("enum");
        if (KeyType->IsA<UBlackboardKeyType_NativeEnum>()) return TEXT("native_enum");
        if (KeyType->IsA<UBlackboardKeyType_Struct>())     return TEXT("struct");
        return KeyType->GetClass()->GetName();
    }
}

// ============================================================================
// BehaviorTree find / serialize
// ============================================================================

UBehaviorTree* FBehaviorTreeService::FindBehaviorTree(const FString& Name)
{
    return FindAssetByPathOrName<UBehaviorTree>(Name);
}

void FBehaviorTreeService::SerializeNodeBase(UBTNode* Node, const TSharedPtr<FJsonObject>& OutObj) const
{
    if (!Node || !OutObj.IsValid())
    {
        return;
    }

    OutObj->SetStringField(TEXT("node_name"), Node->NodeName);
    UClass* NodeClass = Node->GetClass();
    OutObj->SetStringField(TEXT("class"), NodeClass ? NodeClass->GetName() : TEXT(""));
    OutObj->SetStringField(TEXT("class_path"), NodeClass ? NodeClass->GetPathName() : TEXT(""));

    // For Blueprint-derived nodes (BTT_*, BTD_*, BTS_* assets), expose the source Blueprint
    // so the caller can call get_blueprint_metadata on it.
    if (NodeClass)
    {
#if WITH_EDITOR
        if (UBlueprint* SourceBP = UBlueprint::GetBlueprintFromClass(NodeClass))
        {
            OutObj->SetStringField(TEXT("blueprint_path"), SourceBP->GetPathName());
            OutObj->SetBoolField(TEXT("uses_blueprint"), true);
        }
        else
#endif
        {
            OutObj->SetBoolField(TEXT("uses_blueprint"), false);
        }

#if WITH_EDITOR
        const FString StaticDesc = Node->GetStaticDescription();
        if (!StaticDesc.IsEmpty())
        {
            OutObj->SetStringField(TEXT("static_description"), StaticDesc);
        }
#endif
    }
}

TSharedPtr<FJsonObject> FBehaviorTreeService::SerializeDecorator(UBTDecorator* Decorator) const
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    if (!Decorator)
    {
        return Obj;
    }
    Obj->SetStringField(TEXT("node_type"), TEXT("decorator"));
    SerializeNodeBase(Decorator, Obj);
    return Obj;
}

TSharedPtr<FJsonObject> FBehaviorTreeService::SerializeService(UBTService* Service) const
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    if (!Service)
    {
        return Obj;
    }
    Obj->SetStringField(TEXT("node_type"), TEXT("service"));
    SerializeNodeBase(Service, Obj);
    return Obj;
}

TSharedPtr<FJsonObject> FBehaviorTreeService::SerializeTask(UBTTaskNode* Task) const
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    if (!Task)
    {
        return Obj;
    }
    Obj->SetStringField(TEXT("node_type"), TEXT("task"));
    SerializeNodeBase(Task, Obj);
    return Obj;
}

TSharedPtr<FJsonObject> FBehaviorTreeService::SerializeComposite(UBTCompositeNode* Composite) const
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    if (!Composite)
    {
        return Obj;
    }
    Obj->SetStringField(TEXT("node_type"), TEXT("composite"));
    SerializeNodeBase(Composite, Obj);

    // Services attached to this composite.
    TArray<TSharedPtr<FJsonValue>> ServiceArray;
    for (UBTService* Service : Composite->Services)
    {
        ServiceArray.Add(MakeShared<FJsonValueObject>(SerializeService(Service)));
    }
    Obj->SetArrayField(TEXT("services"), ServiceArray);

    // Children: each FBTCompositeChild has decorators + either a child composite or task.
    TArray<TSharedPtr<FJsonValue>> ChildrenArray;
    for (const FBTCompositeChild& Child : Composite->Children)
    {
        TSharedPtr<FJsonObject> ChildObj = MakeShared<FJsonObject>();

        // Per-child decorators.
        TArray<TSharedPtr<FJsonValue>> DecArray;
        for (UBTDecorator* Decorator : Child.Decorators)
        {
            DecArray.Add(MakeShared<FJsonValueObject>(SerializeDecorator(Decorator)));
        }
        ChildObj->SetArrayField(TEXT("decorators"), DecArray);

        // Decorator logic ops (AND/OR/NOT). Often empty for single-decorator chains.
        TArray<TSharedPtr<FJsonValue>> OpArray;
        for (const FBTDecoratorLogic& Op : Child.DecoratorOps)
        {
            TSharedPtr<FJsonObject> OpObj = MakeShared<FJsonObject>();
            OpObj->SetNumberField(TEXT("operation"), static_cast<int32>(Op.Operation));
            OpObj->SetNumberField(TEXT("number"), Op.Number);
            OpArray.Add(MakeShared<FJsonValueObject>(OpObj));
        }
        ChildObj->SetArrayField(TEXT("decorator_ops"), OpArray);

        // The actual child node — recurse if composite, terminate if task.
        if (Child.ChildComposite)
        {
            ChildObj->SetObjectField(TEXT("node"), SerializeComposite(Child.ChildComposite));
        }
        else if (Child.ChildTask)
        {
            ChildObj->SetObjectField(TEXT("node"), SerializeTask(Child.ChildTask));
        }

        ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildObj));
    }
    Obj->SetArrayField(TEXT("children"), ChildrenArray);

    // SimpleParallel composites have a MainTask separate from Children — surface it.
    if (UBTComposite_SimpleParallel* Parallel = Cast<UBTComposite_SimpleParallel>(Composite))
    {
        if (Parallel->Children.Num() > 0 && Parallel->Children[0].ChildTask)
        {
            Obj->SetObjectField(TEXT("main_task"), SerializeTask(Parallel->Children[0].ChildTask));
        }
    }

    return Obj;
}

TSharedPtr<FJsonObject> FBehaviorTreeService::SerializeAnyNode(UBTNode* Node) const
{
    if (UBTCompositeNode* Comp = Cast<UBTCompositeNode>(Node))
    {
        return SerializeComposite(Comp);
    }
    if (UBTTaskNode* Task = Cast<UBTTaskNode>(Node))
    {
        return SerializeTask(Task);
    }
    if (UBTDecorator* Dec = Cast<UBTDecorator>(Node))
    {
        return SerializeDecorator(Dec);
    }
    if (UBTService* Svc = Cast<UBTService>(Node))
    {
        return SerializeService(Svc);
    }
    return MakeShared<FJsonObject>();
}

bool FBehaviorTreeService::GetBehaviorTreeMetadata(UBehaviorTree* BehaviorTree, TSharedPtr<FJsonObject>& OutMetadata)
{
    if (!BehaviorTree)
    {
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), BehaviorTree->GetName());
    OutMetadata->SetStringField(TEXT("path"), BehaviorTree->GetPathName());

    if (BehaviorTree->BlackboardAsset)
    {
        OutMetadata->SetStringField(TEXT("blackboard_path"), BehaviorTree->BlackboardAsset->GetPathName());
    }

    // Root entry decorators (applied to the root before the root composite executes).
    TArray<TSharedPtr<FJsonValue>> RootDecArray;
    for (UBTDecorator* Decorator : BehaviorTree->RootDecorators)
    {
        RootDecArray.Add(MakeShared<FJsonValueObject>(SerializeDecorator(Decorator)));
    }
    OutMetadata->SetArrayField(TEXT("root_decorators"), RootDecArray);

    TArray<TSharedPtr<FJsonValue>> RootOpArray;
    for (const FBTDecoratorLogic& Op : BehaviorTree->RootDecoratorOps)
    {
        TSharedPtr<FJsonObject> OpObj = MakeShared<FJsonObject>();
        OpObj->SetNumberField(TEXT("operation"), static_cast<int32>(Op.Operation));
        OpObj->SetNumberField(TEXT("number"), Op.Number);
        RootOpArray.Add(MakeShared<FJsonValueObject>(OpObj));
    }
    OutMetadata->SetArrayField(TEXT("root_decorator_ops"), RootOpArray);

    if (BehaviorTree->RootNode)
    {
        OutMetadata->SetObjectField(TEXT("root"), SerializeComposite(BehaviorTree->RootNode));
    }

    return true;
}

// ============================================================================
// Blackboard find / serialize
// ============================================================================

UBlackboardData* FBehaviorTreeService::FindBlackboard(const FString& Name)
{
    return FindAssetByPathOrName<UBlackboardData>(Name);
}

bool FBehaviorTreeService::GetBlackboardMetadata(UBlackboardData* Blackboard, TSharedPtr<FJsonObject>& OutMetadata)
{
    if (!Blackboard)
    {
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), Blackboard->GetName());
    OutMetadata->SetStringField(TEXT("path"), Blackboard->GetPathName());
    if (Blackboard->Parent)
    {
        OutMetadata->SetStringField(TEXT("parent_path"), Blackboard->Parent->GetPathName());
    }

    // Walk the Parent chain, child entries take precedence over parent entries with the
    // same name (this matches runtime resolution).
    TSet<FName> SeenNames;
    TArray<TSharedPtr<FJsonValue>> KeyArray;

    auto AppendKeys = [&](UBlackboardData* Source, const FString& OwnerPath, bool bInherited)
    {
        for (const FBlackboardEntry& Entry : Source->Keys)
        {
            if (SeenNames.Contains(Entry.EntryName))
            {
                continue;
            }
            SeenNames.Add(Entry.EntryName);

            TSharedPtr<FJsonObject> KeyObj = MakeShared<FJsonObject>();
            KeyObj->SetStringField(TEXT("name"), Entry.EntryName.ToString());
            KeyObj->SetStringField(TEXT("type"), BlackboardKeyTypeName(Entry.KeyType));
            KeyObj->SetStringField(TEXT("type_class"), Entry.KeyType ? Entry.KeyType->GetClass()->GetPathName() : TEXT(""));
            KeyObj->SetBoolField(TEXT("instance_synced"), Entry.bInstanceSynced);
            KeyObj->SetBoolField(TEXT("inherited"), bInherited);
            if (bInherited)
            {
                KeyObj->SetStringField(TEXT("inherited_from"), OwnerPath);
            }

#if WITH_EDITORONLY_DATA
            if (!Entry.EntryDescription.IsEmpty())
            {
                KeyObj->SetStringField(TEXT("description"), Entry.EntryDescription);
            }
            if (!Entry.EntryCategory.IsNone())
            {
                KeyObj->SetStringField(TEXT("category"), Entry.EntryCategory.ToString());
            }
#endif

            // Type-specific extras.
            if (auto* ObjType = Cast<UBlackboardKeyType_Object>(Entry.KeyType))
            {
                KeyObj->SetStringField(TEXT("base_class"), ObjType->BaseClass ? ObjType->BaseClass->GetPathName() : TEXT(""));
            }
            else if (auto* ClassType = Cast<UBlackboardKeyType_Class>(Entry.KeyType))
            {
                KeyObj->SetStringField(TEXT("base_class"), ClassType->BaseClass ? ClassType->BaseClass->GetPathName() : TEXT(""));
            }
            else if (auto* EnumType = Cast<UBlackboardKeyType_Enum>(Entry.KeyType))
            {
                if (EnumType->EnumType)
                {
                    KeyObj->SetStringField(TEXT("enum_type"), EnumType->EnumType->GetPathName());
                    KeyObj->SetStringField(TEXT("enum_name"), EnumType->EnumName);
                }
            }
            // Struct key type: don't probe its DefaultValue (API has shifted across UE versions);
            // the type_class field above already identifies it. Add later if needed.

            KeyArray.Add(MakeShared<FJsonValueObject>(KeyObj));
        }
    };

    // Child first (takes precedence), then walk up.
    AppendKeys(Blackboard, Blackboard->GetPathName(), false);
    for (UBlackboardData* Ancestor = Blackboard->Parent; Ancestor; Ancestor = Ancestor->Parent)
    {
        AppendKeys(Ancestor, Ancestor->GetPathName(), true);
    }

    OutMetadata->SetArrayField(TEXT("keys"), KeyArray);
    OutMetadata->SetNumberField(TEXT("num_keys"), KeyArray.Num());

    return true;
}
