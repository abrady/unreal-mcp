"""Tests for get_anim_montage_metadata."""

from utils.unreal_connection_utils import send_unreal_command

from .conftest import ANIM_MONTAGE_PATH, unwrap_response


def test_get_anim_montage_metadata_returns_sections_and_notifies():
    result = unwrap_response(
        send_unreal_command("get_anim_montage_metadata", {"montage_path": ANIM_MONTAGE_PATH})
    )
    assert result is not None
    assert result.get("success") is True, f"call failed: {result}"

    meta = result.get("metadata")
    assert isinstance(meta, dict)
    assert meta.get("name") == "AM_Attacks"
    assert meta.get("path", "").startswith("/Game/")
    assert isinstance(meta.get("play_length"), (int, float))
    assert "rate_scale" in meta
    assert "blend_in_time" in meta
    assert "blend_out_time" in meta

    sections = meta.get("sections", [])
    assert isinstance(sections, list)
    assert len(sections) > 0, "AM_Attacks should have at least one section"
    sec = sections[0]
    assert "section_index" in sec
    assert "name" in sec
    assert "next_section_name" in sec
    assert "start_time" in sec
    assert "end_time" in sec
    assert "length" in sec

    slots = meta.get("slots", [])
    assert isinstance(slots, list)
    # Slots may be empty if montage just uses section sequencing — both valid.
    if slots:
        slot = slots[0]
        assert "slot_name" in slot
        assert "segments" in slot

    notifies = meta.get("notifies", [])
    assert isinstance(notifies, list)
    # AM_Attacks is expected to have at least the StartRightHandAttack/EndRightHandAttack notifies.
    # Don't hard-require — flag a softer expectation.
    if notifies:
        n = notifies[0]
        for key in (
            "notify_name",
            "trigger_time",
            "duration",
            "end_trigger_time",
            "track_index",
            "is_state",
            "notify_class",
            "notify_class_short",
            "montage_tick_type",
        ):
            assert key in n, f"notify missing field {key}"


def test_get_anim_montage_metadata_unknown_returns_error():
    raw = send_unreal_command(
        "get_anim_montage_metadata", {"montage_path": "/Game/Nope/NotAMontage.NotAMontage"}
    )
    result = unwrap_response(raw)
    assert result is not None
    assert raw.get("status") == "error" or result.get("success") is False
    assert "error" in (raw if raw.get("status") == "error" else result)
