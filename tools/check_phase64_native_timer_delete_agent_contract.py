#!/usr/bin/env python3
"""Architecture guard for Phase 64 Slice 24 Timer-delete Agent contract."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/agent/include/BackendAgentNativeTimerDelete.h"
SOURCE = ROOT / "core/agent/src/BackendAgentNativeTimerDelete.cpp"
TEST = ROOT / "core/agent/tests/test_backend_agent_native_timer_delete.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-delete-agent-contract.md"
FRAGMENT = ROOT / "mk/phase64-native-timer-delete-agent-contract-tests.mk"
MAKEFILE = ROOT / "Makefile"

failures: list[str] = []


def require(path: Path, markers: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing Slice-24 file: {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            failures.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")


require(HEADER, [
    '"vdr.timer.delete"',
    '"vdr.timer"',
    '"suitebridge:local"',
    "BackendAgentNativeTimerDeleteCommand",
    "BackendAgentNativeTimerDeleteEvidence",
    "operationRevision",
    "expectedBindingRevision",
    "controlPlaneClaimedAt",
    "localStartingPersistedAt",
    "acceptedUnverified",
    "outcomeUnknown",
])
require(SOURCE, [
    "backendAgentLocalProviderValidSelection",
    "kBackendAgentNativeTimerDeleteAuthorityDomain",
    "kBackendAgentNativeTimerDeleteCapability",
    "kBackendAgentNativeTimerDeleteProviderId",
    "kBackendAgentNativeTimerDeleteProviderKind",
    "evidence.localStartingPersistedAt < command.controlPlaneClaimedAt",
    "evidence.dispatchStartedAt < evidence.localStartingPersistedAt",
    "rejected-outcome-has-dispatch",
])
require(TEST, [
    'requiredCapability = "vdr.native.probe"',
    'authorityDomain = "vdr.native"',
    'providerId = "restfulapi:local"',
    "acceptedUnverified",
    "outcomeUnknown",
    "rejectedWithoutEffect",
    "startingBeforeClaim",
    "wrongOperation",
    "wrongEpoch",
])
require(DOC, [
    "Phase 64 Slice 24",
    "PR #176",
    "vdr.timer.delete",
    "Availability is not authority",
    "starting",
    "no runtime wiring",
    "mutations remain disabled",
    "Slice 25",
])
require(FRAGMENT, [
    "test-phase64-native-timer-delete-agent-contract",
    "BackendAgentLocalProvider.cpp",
    "BackendAgentNativeTimerDelete.cpp",
    "test_backend_agent_native_timer_delete.cpp",
    "test-fast: test-phase64-native-timer-delete-agent-contract",
])
require(MAKEFILE, ["include mk/phase64-native-timer-delete-agent-contract-tests.mk"])

source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
for forbidden in [
    "BackendAgentCommandClient",
    "ISuiteBridgeLocalTransport",
    "SuiteBridgeCommandReply",
    "SVDRP",
    "RESTfulAPI",
    "sqlite3_",
    "system(",
    "popen(",
]:
    if forbidden in source:
        failures.append(f"Slice-24 contract contains forbidden runtime/write coupling: {forbidden}")

for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "BackendAgentNativeTimerDelete.cpp" in text:
        failures.append(
            f"Slice-24 contract is unexpectedly wired into runtime sources: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 native Timer delete Agent contract check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer delete Agent contract check passed")
