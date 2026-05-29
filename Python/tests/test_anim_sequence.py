"""Tests for get_anim_sequence_metadata."""

from utils.unreal_connection_utils import send_unreal_command

from .conftest import unwrap_response

# IceWarden idle exists in the Shin and the Guardians of Spirits project (verified
# during prior investigation under /Game/Characters/Bosses/IceBoss/).
ANIM_SEQUENCE_PATH = (
    "/Game/Characters/Bosses/IceBoss/IceBossArmature_IceWarden_Idle.IceBossArmature_IceWarden_Idle"
)


def test_get_anim_sequence_metadata_returns_core_fields():
    result = unwrap_response(
        send_unreal_command("get_anim_sequence_metadata", {"sequence_path": ANIM_SEQUENCE_PATH})
    )
    assert result is not None
    assert result.get("success") is True, f"call failed: {result}"

    meta = result.get("metadata")
    assert isinstance(meta, dict)
    assert meta.get("name") == "IceBossArmature_IceWarden_Idle"
    assert meta.get("path", "").startswith("/Game/")
    assert isinstance(meta.get("play_length"), (int, float))
    assert meta.get("play_length") > 0
    assert "skeleton_path" in meta
    assert "notifies" in meta and isinstance(meta["notifies"], list)
    assert "sync_markers" in meta and isinstance(meta["sync_markers"], list)


def test_get_anim_sequence_metadata_unknown_returns_error():
    raw = send_unreal_command(
        "get_anim_sequence_metadata", {"sequence_path": "/Game/Nope/NotASeq.NotASeq"}
    )
    result = unwrap_response(raw)
    assert result is not None
    assert raw.get("status") == "error" or result.get("success") is False
    assert "error" in (raw if raw.get("status") == "error" else result)
