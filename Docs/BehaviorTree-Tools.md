# BehaviorTree Tools - Unreal MCP

This document covers the BehaviorTree-domain MCP tools — **read-only inspection** of `UBehaviorTree` and `UBlackboardData` assets. Neither asset class is a `UBlueprint`, so the existing `find_in_blueprints` tool cannot reach their structure; these tools close that gap.

## Overview

BehaviorTree tools enable you to:
- Dump the full recursive node tree of any `UBehaviorTree` asset (composites, decorators, services, tasks)
- Surface the source-Blueprint path for `BTT_*` / `BTD_*` / `BTS_*` Blueprint-derived nodes so callers can chain into `get_blueprint_metadata`
- Walk the parent chain of any `UBlackboardData` asset and dump every key with its canonical type
- Surface `WITH_EDITOR` static descriptions (e.g. `"Blackboard: AI State Is Equal To Attack"`) when available

These tools are *strictly* read-only — there are no `create_*` or `set_*` variants for the BehaviorTree domain. If you need to modify a BT, do so in the editor; these tools exist to help you *understand* what's already there.

## Natural Language Usage Examples

### Inspecting a BehaviorTree

```
"Show me the node tree of /Game/AI/BT_Boss — I want to see what selectors and tasks are wired up"

"What services run on the root selector of BT_Enemy?"

"Which Blueprint tasks does BT_FireGolem call into?"

"Dump the decorators on every child of the root Selector"
```

### Inspecting a Blackboard

```
"List every key on BB_Memory with its type"

"Does BB_Boss inherit any keys from a parent blackboard?"

"What's the base class restriction on the TargetActor key?"
```

## Tool Reference

---

### `get_behavior_tree_metadata`

Read-only. Walks `UBehaviorTree::RootNode` recursively and serializes the full node graph: composites → children (with per-child decorator chains) + services → tasks. For Blueprint-derived nodes (`BTT_*_C`, `BTD_*_C`, `BTS_*_C`), the source `.uasset` path is included so callers can follow up with `get_blueprint_metadata`.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `bt_path` | string | ✅ | Full asset path (`"/Game/.../BT_Foo.BT_Foo"`) or bare asset name. Path form is preferred. |

Returns `{ success, metadata: { ... } }`. The `metadata` object contains:

- `name`, `path` — asset identity
- `blackboard_path` — the BB this BT references (may be empty)
- `root_decorators[]`, `root_decorator_ops[]` — decorators applied above the root composite
- `root` — recursive composite-node object

Each node in the recursion has:

| Field | Notes |
|---|---|
| `node_type` | `"composite"` / `"task"` / `"decorator"` / `"service"` |
| `node_name` | Display name from `UBTNode::NodeName` |
| `class` / `class_path` | Short class name + full `/Script/...` or `/Game/...` path |
| `uses_blueprint` | `true` for `BTT_*_C` Blueprint tasks etc. |
| `blueprint_path` | Set when `uses_blueprint=true` — the `.uasset` path to feed into `get_blueprint_metadata` |
| `static_description` | Editor-side description (`WITH_EDITOR` only) — often the most readable summary, e.g. `"BTS_CheckForTargetDistance: tick every 0.90s..1.10s"` |

Composite nodes additionally include:
- `services[]` — services that run while the composite is active
- `children[]` — each `{ decorators: [...], decorator_ops: [...], node: { ... } }`
- `main_task` — for `UBTComposite_SimpleParallel` only

**Example:**
```
get_behavior_tree_metadata(bt_path="/Game/AI/BT_Boss.BT_Boss")
```

---

### `get_blackboard_metadata`

Read-only. Walks a `UBlackboardData` asset's `Keys` array plus its `Parent` chain, deduplicating on `EntryName` (child entries take precedence on collision — this matches runtime resolution).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `blackboard_path` | string | ✅ | Full asset path (`"/Game/.../BB_Foo.BB_Foo"`) or bare asset name. |

Returns `{ success, metadata: { name, path, parent_path?, num_keys, keys: [...] } }` where each key has:

| Field | Notes |
|---|---|
| `name` | `EntryName.ToString()` — beware of spaces (`"Target Actor"` is common) |
| `type` | Canonical lowercase: `"bool"`, `"int"`, `"float"`, `"string"`, `"name"`, `"vector"`, `"rotator"`, `"object"`, `"class"`, `"enum"`, `"native_enum"`, `"struct"` |
| `type_class` | Full path of the `UBlackboardKeyType_*` subclass |
| `instance_synced` | Whether the value syncs across AI instances |
| `inherited` | `true` when the key originated on a parent blackboard |
| `inherited_from` | Parent BB path when `inherited=true` |
| `description`, `category` | Editor-only metadata when available |
| `base_class` | For `object` / `class` types — the restricted base class |
| `enum_type`, `enum_name` | For `enum` types — the source enum path and FName |

**Example:**
```
get_blackboard_metadata(blackboard_path="/Game/AI/BB_Boss.BB_Boss")
```

## Common Workflows

**"What does this boss actually do?"**
1. `get_behavior_tree_metadata(bt_path="/Game/AI/BT_Boss.BT_Boss")` — see the node layout
2. For each `BTT_*_C` task found, follow up with `get_blueprint_metadata(blueprint_name="BTT_AttackPlayer")` to see its variables and event graph entry points
3. `get_blackboard_metadata(...)` on the BT's `blackboard_path` to see what state the BT reads / writes

**"Why isn't my decorator firing?"**
1. `get_behavior_tree_metadata(...)` and look for the decorator's `static_description` — that's the human-readable rule string
2. Check the decorator's parent child wrapper — `decorator_ops` shows any AND/OR/NOT logic between sibling decorators
3. Check the BB key the decorator references via `get_blackboard_metadata(...)` — confirm the key exists with the expected type

**"What key name does the BT actually use for the player target?"**
Common Blueprint authoring mistake: a key authored as `"Target Actor"` (with space) gets typed as `"TargetActor"` or `"Player"` in BTT code. Use `get_blackboard_metadata` to read the canonical key name from the asset, then verify the BTT Blueprint reads that exact `FName`.

## Limitations

- The recursive node walk operates on `UBehaviorTree::RootNode` and its compiled tree — not the `UEdGraph` editor representation. This means it sees what the runtime sees, but cosmetic-only data (node positions, comment boxes) is omitted by design.
- For Blueprint-derived nodes, internal variable defaults are NOT included here — call `get_blueprint_metadata(blueprint_name=...)` on the `blueprint_path` field to get those.
- `UBlackboardKeyType_Struct` is identified by `type` and `type_class` but the struct's default value isn't probed (the `DefaultValue` API has shifted across UE versions; we surface the type so the caller can identify it).
- Both tools require the editor to be running with the UnrealMCP plugin loaded — they hit `127.0.0.1:55557` like every other MCP tool in this suite.

## Related Tools

- [`get_blueprint_metadata`](Blueprint-Tools.md) — follow `blueprint_path` from a Blueprint-derived BT node into its source Blueprint
- [`get_anim_montage_metadata`](Animation-Tools.md) — for `BTT_PlayMontage`-style tasks, inspect the montage they reference
- [`dump_asset`](Editor-Tools.md) — universal fallback for asset types not yet covered by typed metadata tools
- [`get_state_tree_metadata`](StateTree-Tools.md) — StateTree is the modern successor to BehaviorTree for many use cases
