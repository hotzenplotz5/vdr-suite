#!/usr/bin/env python3
"""Static guard for the historical Phase-63 Slice-2 observation contract.

This guard intentionally protects only immutable Phase-63 contract/evidence.
Current/latest/next phase status belongs to docs/CURRENT.md and the generic
phase-status guards; historical guards must not become volatile status owners.
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CLOSEOUT = ROOT / "docs/development/phase-63-slice-1-closeout.md"
CONTRACT = ROOT / "docs/development/phase-63-observation-ingestion.md"
SLICE2_CLOSEOUT = ROOT / "docs/development/phase-63-slice-2-closeout.md"

failures: list[str] = []


def require(path: Path, markers: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing Phase-63 contract file: {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    folded_text = text.casefold()
    for marker in markers:
        if marker.casefold() not in folded_text:
            failures.append(
                f"{path.relative_to(ROOT)} misses required marker: {marker}"
            )


require(
    CLOSEOUT,
    [
        "Phase 63 Slice 1 is completed",
        "PR #137",
        "a9620179a442155f0860ef3182ca39186ac46a57",
        "bba51455552bab0f1a06c680369c508858b2384b",
        "575f49a197cda9ad02da4035b437ee1c32bed2d6",
        "VDR-Suite CI #7256",
        "PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS",
        "VDR_NATIVE_STATE_UNCHANGED=yes",
        "Phase 63 Slice 2",
    ],
)

require(
    CONTRACT,
    [
        "vdr-suite-agent/1",
        "backendGeneration",
        "agentInstanceId",
        "observationDomain",
        "snapshotGeneration",
        "producerSequence",
        "resourceRevision",
        "completeSnapshot",
        "changeBatch",
        "resync-required",
        "equivalent replay",
        "conflicting replay",
        "Suite-owned repositories and transactions",
        "repository layer owns SQLite",
        "/api/agent/v1/observations/",
        "backend-health",
        "no VDR-native mutation",
        "No manual SQLite inspection",
        "command inbox",
        "provider ownership",
        "Phase 64",
    ],
)

require(
    SLICE2_CLOSEOUT,
    [
        "24b1d7938ddaa15834a8da6323a270761868f4ba",
        "PR #138",
        "PR #139",
        "Phase 63 Slice 2",
        "backend-health",
    ],
)

contract_text = CONTRACT.read_text(encoding="utf-8") if CONTRACT.is_file() else ""
for forbidden in [
    "implement command dispatch in this slice",
    "public Agent URL",
    "replace BackendNode.online authority",
    "manual SQLite inspection is required",
]:
    if forbidden in contract_text:
        failures.append(f"observation contract contains forbidden scope: {forbidden}")

if failures:
    print("Phase-63 observation ingestion contract check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 observation ingestion contract check passed")
print("Historical Slice-2 contract merge: 24b1d7938ddaa15834a8da6323a270761868f4ba")
print("Current phase status intentionally delegated to docs/CURRENT.md")
