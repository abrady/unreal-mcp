"""Tests for get_blackboard_metadata."""

from utils.unreal_connection_utils import send_unreal_command

from .conftest import BLACKBOARD_PATH, unwrap_response


KNOWN_BB_MEMORY_KEYS = {"SelfActor", "Target Actor", "Target Location", "Distance To Target", "AI State"}


def test_get_blackboard_metadata_returns_typed_keys():
    result = unwrap_response(
        send_unreal_command("get_blackboard_metadata", {"blackboard_path": BLACKBOARD_PATH})
    )
    assert result is not None
    assert result.get("success") is True, f"call failed: {result}"

    meta = result.get("metadata")
    assert isinstance(meta, dict)
    assert meta.get("name") == "BB_Memory"
    assert meta.get("path", "").startswith("/Game/")
    assert "num_keys" in meta

    keys = meta.get("keys", [])
    assert isinstance(keys, list)
    assert len(keys) > 0, "BB_Memory should have keys"

    # Every key has the documented shape.
    for k in keys:
        assert "name" in k
        assert "type" in k
        assert "type_class" in k
        assert "instance_synced" in k
        assert "inherited" in k

    # We expect at least one of the keys we found via prior investigation.
    found_names = {k["name"] for k in keys}
    assert KNOWN_BB_MEMORY_KEYS & found_names, (
        f"Expected to find at least one of {KNOWN_BB_MEMORY_KEYS}, got: {found_names}"
    )


def test_get_blackboard_metadata_unknown_returns_error():
    raw = send_unreal_command(
        "get_blackboard_metadata", {"blackboard_path": "/Game/Nope/NotABB.NotABB"}
    )
    result = unwrap_response(raw)
    assert result is not None
    assert raw.get("status") == "error" or result.get("success") is False
    assert "error" in (raw if raw.get("status") == "error" else result)
