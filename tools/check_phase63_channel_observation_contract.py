#!/usr/bin/env python3
"""Static guard for the Phase-63 read-only Channel observation contract."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs/development/phase-63-channel-observation-ingestion.md"
GENERIC_CONTRACT = ROOT / "docs/development/phase-63-observation-ingestion.md"
LIFECYCLE = ROOT / "core/agent/src/BackendAgentLifecycle.cpp"
HTTP_SERVER = ROOT / "core/agent/src/BackendAgentHttpServer.cpp"
CLIENT = ROOT / "core/agent/include/BackendAgentClient.h"
SCHEMA = ROOT / "database/schema/vdr-suite.sql"
MAKE_FRAGMENT = ROOT / "mk/phase63-runtime-acceptance.mk"

BASE_COMMIT = "37df59552fd6d2f739c580dc9b472416f0bf5a12"
BASE_TREE = "71a33d2864aa921a2a82a3c7c1f85b1ddfd7056b"

failures: list[str] = []


def read(path: Path) -> str:
    if not path.is_file():
        failures.append(f"missing required file: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8")


def require(path: Path, markers: list[str]) -> None:
    text = read(path)
    folded = text.casefold()
    for marker in markers:
        if marker.casefold() not in folded:
            failures.append(
                f"{path.relative_to(ROOT)} misses required marker: {marker}"
            )


require(
    CONTRACT,
    [
        "contract-only slice",
        BASE_COMMIT,
        BASE_TREE,
        "channels",
        "(backendId, channelId)",
        "channelNumber",
        "groupName",
        "completeSnapshot",
        "producerSequence = 1",
        "changeBatch",
        "upserts",
        "removedChannelIds",
        "equivalent replay",
        "conflicting replay",
        "resync-required",
        "deterministic canonicalization",
        "explicit site-local read source",
        "source failure",
        "Agent-owned channels observation lineage",
        "VdrChannelCacheRepository",
        "vdr_channel_cache",
        "provider ownership",
        "provider selection",
        "Suite-owned transaction",
        "repository classes own SQLite",
        "no real-system installation",
        "VDR-native channel fingerprints remain unchanged",
        "no enrollment, revocation, Agent replacement",
        "no manual SQLite inspection",
        "command inbox",
        "Phase 64",
        "separate bounded Draft runtime PR",
    ],
)

require(
    GENERIC_CONTRACT,
    [
        "vdr-suite-agent/1",
        "snapshotGeneration",
        "producerSequence",
        "resourceRevision",
        "completeSnapshot",
        "changeBatch",
        "resync-required",
        "backend-health",
        "No manual SQLite inspection",
    ],
)

require(
    MAKE_FRAGMENT,
    [
        "test-phase63-channel-observation-contract",
        "tools/check_phase63_channel_observation_contract.py",
    ],
)

# The contract remains binding after the separately reviewed runtime is added.
# Runtime-specific scope and architecture are guarded independently.

contract_text = read(CONTRACT)
for forbidden in [
    "Agent channels replace direct-adapter authority",
    "missing channels imply deletion in a change batch",
    "source failure is an empty snapshot",
    "public Agent URL is required",
    "manual SQLite inspection is required",
    "implement channel commands in this slice",
]:
    if forbidden in contract_text:
        failures.append(f"Channel contract contains forbidden scope: {forbidden}")

if failures:
    print("Phase-63 Channel observation contract check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 Channel observation contract check passed")
print(f"Merged backend-health runtime base: {BASE_COMMIT}")
print(f"Merged backend-health runtime tree: {BASE_TREE}")
print("Next bounded contract domain: channels")
print("Runtime implementation: guarded separately when present")
