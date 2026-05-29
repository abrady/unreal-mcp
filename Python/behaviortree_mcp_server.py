#!/usr/bin/env python3
"""
BehaviorTree MCP Server

Read-only inspection of UBehaviorTree and UBlackboardData assets. Neither is a
UBlueprint, so find_in_blueprints can't reach them — this server fills that gap.
"""

import asyncio
import json
from typing import Any, Dict

from fastmcp import FastMCP

app = FastMCP("BehaviorTree MCP Server")

TCP_HOST = "127.0.0.1"
TCP_PORT = 55557


async def send_tcp_command(command_type: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to the Unreal Engine TCP server."""
    try:
        command_data = {"type": command_type, "params": params}
        json_data = json.dumps(command_data)

        reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)
        writer.write(json_data.encode("utf-8"))
        writer.write(b"\n")
        await writer.drain()

        # 256KB buffer — get_behavior_tree_metadata on a large tree can grow.
        response_data = await reader.read(262144)
        response_str = response_data.decode("utf-8").strip()

        writer.close()
        await writer.wait_closed()

        if response_str:
            try:
                return json.loads(response_str)
            except json.JSONDecodeError as json_err:
                return {
                    "success": False,
                    "error": f"JSON decode error: {str(json_err)}, Response: {response_str[:200]}",
                }
        return {"success": False, "error": "Empty response from server"}

    except Exception as e:
        return {"success": False, "error": f"TCP communication error: {str(e)}"}


@app.tool()
async def get_behavior_tree_metadata(bt_path: str) -> Dict[str, Any]:
    """
    Dump a UBehaviorTree's node graph as JSON.

    Read-only. Walks the root composite recursively, surfacing decorators,
    services, and tasks (including Blueprint-derived task / decorator /
    service assets — their source blueprint path is included so you can
    follow up with get_blueprint_metadata).

    Args:
        bt_path: Full asset path ("/Game/.../BT_Foo.BT_Foo") or bare asset
                 name. Path form is preferred.

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - metadata: Object containing:
            - name, path: Asset identity
            - blackboard_path: Path to the BB this BT uses
            - root_decorators: Decorators applied above the root
            - root_decorator_ops: Decorator-logic ops for the root chain
            - root: Recursive composite node tree. Each node has:
                * node_type ("composite" | "task" | "decorator" | "service")
                * node_name, class, class_path
                * uses_blueprint, blueprint_path (for BP-derived nodes)
                * static_description (editor-side string when available)
                * For composites: services[], children[]
                  - Each child has decorators[], decorator_ops[], node{}
                * For SimpleParallel composites: main_task{}

    Examples:
        get_behavior_tree_metadata(bt_path="/Game/AI/BT_Boss.BT_Boss")
    """
    return await send_tcp_command("get_behavior_tree_metadata", {"bt_path": bt_path})


@app.tool()
async def get_blackboard_metadata(blackboard_path: str) -> Dict[str, Any]:
    """
    Dump a UBlackboardData asset's keys with type information.

    Read-only. Walks the Parent chain and annotates inherited entries.
    Child entries with the same FName as a parent's entry take precedence
    (matching runtime resolution); the parent's hidden entry is skipped.

    Args:
        blackboard_path: Full asset path ("/Game/.../BB_Foo.BB_Foo") or bare
                         asset name. Path form is preferred.

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - metadata: Object containing:
            - name, path: Asset identity
            - parent_path: Path to parent blackboard if any
            - num_keys: Total resolved key count (child + non-overridden parent)
            - keys: Array of per-key objects with:
                * name: Entry name (string from FName)
                * type: Canonical lowercase ("bool", "int", "float", "string",
                        "name", "vector", "rotator", "object", "class", "enum",
                        "native_enum", "struct")
                * type_class: Full class path of the BlackboardKeyType
                * instance_synced: Whether the value syncs across instances
                * inherited: True if this entry was inherited from a parent
                * inherited_from: Parent BB path when inherited
                * description, category: Editor-only metadata when available
                * base_class: For object/class types — the restricted base class
                * enum_type, enum_name: For enum-typed keys

    Examples:
        get_blackboard_metadata(blackboard_path="/Game/AI/BB_Boss.BB_Boss")
    """
    return await send_tcp_command(
        "get_blackboard_metadata", {"blackboard_path": blackboard_path}
    )


if __name__ == "__main__":
    app.run()
