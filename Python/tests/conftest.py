"""Test fixtures for the asset-introspection tools.

These tests talk to a live Unreal editor over TCP 127.0.0.1:55557, so they
will be SKIPPED if no editor is listening. Start the project's Unreal editor
first.

Each test calls the Python tool's `send_tcp_command` helper directly (not via
the FastMCP protocol layer) — this verifies the C++ command handler and the
TCP wire contract without depending on FastMCP-side wiring.
"""

import socket
import sys
from pathlib import Path

import pytest

PY_ROOT = Path(__file__).resolve().parent.parent
if str(PY_ROOT) not in sys.path:
    sys.path.insert(0, str(PY_ROOT))


def _editor_listening(host: str = "127.0.0.1", port: int = 55557, timeout: float = 1.0) -> bool:
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


@pytest.fixture(autouse=True)
def _editor_required():
    """Skip every test in this directory if the editor isn't running."""
    if not _editor_listening():
        pytest.skip(
            "Unreal editor not listening on 127.0.0.1:55557 — start the editor before running these tests"
        )


# ---------- Known asset paths in the Shin and the Guardians of Spirits project ----------

ANIM_MONTAGE_PATH = "/Game/Blueprints/Characters/Enemies/Bosses/FireGolem/AM_Attacks.AM_Attacks"
BEHAVIOR_TREE_PATH = "/Game/Blueprints/Characters/Enemies/AI/NewAI/Behavior/BT_FireGolem.BT_FireGolem"
BLACKBOARD_PATH = "/Game/Blueprints/Characters/Enemies/AI/NewAI/Behavior/BB_Memory.BB_Memory"


def unwrap_response(response):
    """The UnrealMCPBridge wraps each command's JSON inside {"status": "success",
    "result": {...actual response...}} on success and surfaces failures as
    {"status": "error", "error": "..."}. Tests want to assert on the inner
    command-level shape, so peel one layer when present."""
    if response is None:
        return response
    if isinstance(response, dict) and response.get("status") == "success" and "result" in response:
        return response["result"]
    return response
