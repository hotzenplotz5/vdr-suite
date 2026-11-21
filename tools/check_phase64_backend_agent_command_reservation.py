#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "header": "core/agent/include/BackendAgentCommandReservation.h",
    "source": "core/agent/src/BackendAgentCommandReservation.cpp",
    "test": "core/agent/tests/test_backend_agent_command_reservation.cpp",
    "doc": "docs/development/phase-64-backend-agent-command-reservation.md",
    "mk": "mk/phase64-backend-agent-command-reservation-tests.mk",
    "makefile": "Makefile",
}
for relative in paths.values():
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing backend Agent command reservation file: {relative}")

header = (ROOT / paths["header"]).read_text(encoding="utf-8")
source = (ROOT / paths["source"]).read_text(encoding="utf-8")
test = (ROOT / paths["test"]).read_text(encoding="utf-8")
doc = (ROOT / paths["doc"]).read_text(encoding="utf-8")
mk = (ROOT / paths["mk"]).read_text(encoding="utf-8")
makefile = (ROOT / paths["makefile"]).read_text(encoding="utf-8")

for token in [
    "BackendAgentCommandReservationStatus",
    "alreadyReserved",
    "activated",
    "alreadyActivated",
    "BackendAgentCommandReservation",
    "BackendAgentCommandReservationRepository",
    "BackendAgentCommandReservationActivationService",
]:
    if token not in header:
        raise SystemExit(f"missing command reservation header marker: {token}")

for token in [
    "backend_agent_command_reservations",
    "UNIQUE(backend_id,operation_id,command_type)",
    "BEGIN IMMEDIATE TRANSACTION",
    "backendAgentCommandFingerprint(assignment)",
    "backendAgentLocalProviderSelectionIdentity",
    "findAssignmentForOperation",
    "insertAssignment(reservation.assignment, selection)",
    "alreadyActivated",
]:
    if token not in source:
        raise SystemExit(f"missing command reservation source marker: {token}")

for token in [
    "not visible to normal Agent polling/delivery",
    "alreadyReserved",
    "conflict",
    "activated",
    "alreadyActivated",
    "test_backend_agent_command_reservation passed",
]:
    if token not in test:
        raise SystemExit(f"missing command reservation regression marker: {token}")

for token in [
    "not pollable",
    "reserve -> dispatching -> activate",
    "crash",
    "immutable",
    "no blind retry",
    "existing Agent command delivery",
]:
    if token not in doc:
        raise SystemExit(f"missing command reservation documentation marker: {token}")

for token in [
    "test-phase64-backend-agent-command-reservation-architecture",
    "test-phase64-backend-agent-command-reservation:",
    "test-fast: test-phase64-backend-agent-command-reservation",
    "test-architecture: test-phase64-backend-agent-command-reservation-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing command reservation make marker: {token}")

if "include mk/phase64-backend-agent-command-reservation-tests.mk" not in makefile:
    raise SystemExit("backend Agent command reservation make fragment is not included")

# Reservation started as a persistence primitive with no productive runtime
# owner. Phase 69.C may now compose it in the Control-Plane daemon, but only
# behind the dedicated successor guard that proves the CREATE chain remains
# dormant. Agent/client runtime manifests remain forbidden: they must never
# acquire a second reservation authority.
for manifest in [
    ROOT / "mk/agent-sources.mk",
    ROOT / "mk/backend-agent-runtime.mk",
]:
    if manifest.is_file() and "BackendAgentCommandReservation.cpp" in manifest.read_text(
        encoding="utf-8", errors="ignore"
    ):
        raise SystemExit(
            "forbidden command reservation Agent runtime wiring: "
            + str(manifest.relative_to(ROOT))
        )

daemon_manifest = ROOT / "mk/daemon-sources.mk"
if daemon_manifest.is_file() and "BackendAgentCommandReservation.cpp" in daemon_manifest.read_text(
    encoding="utf-8", errors="ignore"
):
    successor_guard = ROOT / "tools/check_phase69_native_timer_create_dispatch_runtime.py"
    if not successor_guard.is_file():
        raise SystemExit(
            "daemon command reservation wiring requires Phase-69.C successor guard")
    successor = successor_guard.read_text(encoding="utf-8")
    for marker in [
        "BackendAgentCommandReservation.cpp",
        "backendAgentCommandReservationRepository_",
        "CREATE dispatch runtime must remain dormant in this slice",
    ]:
        if marker not in successor:
            raise SystemExit(
                "Phase-69.C command reservation successor guard missing marker: "
                + marker)

print("Phase-64 backend Agent command reservation check passed")
