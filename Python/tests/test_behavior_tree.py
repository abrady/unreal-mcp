"""Tests for get_behavior_tree_metadata."""

from utils.unreal_connection_utils import send_unreal_command

from .conftest import BEHAVIOR_TREE_PATH, unwrap_response


def _walk_node_count(node: dict) -> int:
    """Count every node in a behaviour-tree dump (composite + tasks + decorators + services)."""
    if not isinstance(node, dict):
        return 0
    total = 1
    for child in node.get("children", []):
        # decorators / services on the child wrapper
        total += len(child.get("decorators", []))
        if "node" in child and isinstance(child["node"], dict):
            total += _walk_node_count(child["node"])
    total += len(node.get("services", []))
    return total


def test_get_behavior_tree_metadata_returns_root_composite():
    result = unwrap_response(
        send_unreal_command("get_behavior_tree_metadata", {"bt_path": BEHAVIOR_TREE_PATH})
    )
    assert result is not None
    assert result.get("success") is True, f"call failed: {result}"

    meta = result.get("metadata")
    assert isinstance(meta, dict)
    assert meta.get("name") == "BT_FireGolem"
    assert meta.get("path", "").startswith("/Game/")
    # Either a blackboard is set or the field is absent — both fine.
    assert "blackboard_path" in meta or meta.get("blackboard_path") is None

    root = meta.get("root")
    assert isinstance(root, dict), "root should be the root composite node"
    assert root.get("node_type") == "composite"
    assert "node_name" in root
    assert "class" in root
    assert "class_path" in root
    assert "children" in root
    assert isinstance(root.get("children"), list)

    # Sanity: the BT should have at least one node walked.
    assert _walk_node_count(root) >= 1


def test_get_behavior_tree_metadata_unknown_returns_error():
    raw = send_unreal_command("get_behavior_tree_metadata", {"bt_path": "/Game/Nope/NotABT.NotABT"})
    result = unwrap_response(raw)
    assert result is not None
    assert raw.get("status") == "error" or result.get("success") is False
    assert "error" in (raw if raw.get("status") == "error" else result)
