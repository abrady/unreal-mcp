"""Tests for the universal dump_asset tool."""

from utils.unreal_connection_utils import send_unreal_command

from .conftest import ANIM_MONTAGE_PATH, unwrap_response


def test_dump_asset_returns_t3d_text():
    result = unwrap_response(send_unreal_command("dump_asset", {"asset_path": ANIM_MONTAGE_PATH}))
    assert result is not None
    assert result.get("success") is True, f"dump_asset failed: {result}"
    assert result.get("asset_path") == ANIM_MONTAGE_PATH
    assert "class" in result
    dump = result.get("dump", "")
    assert isinstance(dump, str)
    # T3D format always starts with "Begin Object" somewhere in the dump.
    assert "Begin Object" in dump, f"Expected T3D output, got: {dump[:200]}"
    assert "End Object" in dump
    assert result.get("truncated") in (True, False)


def test_dump_asset_unknown_path_returns_error():
    raw = send_unreal_command("dump_asset", {"asset_path": "/Game/Nonexistent/Asset.Asset"})
    result = unwrap_response(raw)
    assert result is not None
    assert raw.get("status") == "error" or result.get("success") is False, f"unexpected: {raw}"


def test_dump_asset_missing_param_returns_error():
    # When the param is missing entirely, the bridge's pre-validate may reject
    # the command before our handler runs — both paths return an error.
    raw = send_unreal_command("dump_asset", {})
    assert raw is not None
    assert raw.get("status") == "error" or (unwrap_response(raw) or {}).get("success") is False
    err_msg = (raw.get("error") or "").lower()
    assert (
        "asset_path" in err_msg
        or "missing" in err_msg
        or "invalid parameters" in err_msg
    ), f"unexpected error: {raw}"
