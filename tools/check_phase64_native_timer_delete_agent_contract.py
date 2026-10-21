#!/usr/bin/env python3
"""Architecture guard for Phase 64 Slice 24 Timer-delete Agent contract."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/agent/include/BackendAgentNativeTimerDelete.h"
FINGERPRINT = ROOT / "core/agent/include/BackendAgentNativeTimerDeleteFingerprint.h"
SOURCE = ROOT / "core/agent/src/BackendAgentNativeTimerDelete.cpp"
TEST = ROOT / "core/agent/tests/test_backend_agent_native_timer_delete.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-delete-agent-contract.md"
FRAGMENT = ROOT / "mk/phase64-native-timer-delete-agent-contract-tests.mk"
MAKEFILE = ROOT / "Makefile"
AGENT_SOURCES = ROOT / "mk/agent-sources.mk"

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
    "expectedNativeTimerFingerprint",
    "controlPlaneClaimedAt",
    "localStartingPersistedAt",
    "acceptedUnverified",
    "outcomeUnknown",
])
require(FINGERPRINT, [
    "kBackendAgentNativeTimerDeleteCanonicalFingerprintMaximum = 4096",
    "kBackendAgentNativeTimerDeleteFingerprintTokenLength = 71",
    'std::string token = "sha256:"',
    "backendAgentNativeTimerDeleteFingerprintToken",
    "backendAgentNativeTimerDeleteFingerprintTokenValid",
])
require(SOURCE, [
    "backendAgentLocalProviderValidSelection",
    "kBackendAgentNativeTimerDeleteAuthorityDomain",
    "kBackendAgentNativeTimerDeleteCapability",
    "kBackendAgentNativeTimerDeleteProviderId",
    "kBackendAgentNativeTimerDeleteProviderKind",
    "backendAgentNativeTimerDeleteFingerprintTokenValid(",
    "evidence.localStartingPersistedAt < command.controlPlaneClaimedAt",
    "evidence.dispatchStartedAt < evidence.localStartingPersistedAt",
    "rejected-outcome-has-dispatch",
])
require(TEST, [
    "std::string fingerprintToken",
    "expectedNativeTimerFingerprint = fingerprintToken()",
    "invalidNativeFingerprint.expectedNativeTimerFingerprint = \"not-hex\"",
    "noNativeFingerprint.expectedNativeTimerFingerprint.clear()",
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

# Slice 24 originally prohibited all runtime wiring. The Slice-29 commands.state
# v3 owner now needs the pure Timer-delete domain validator to validate a typed
# local-state extension. Permit exactly that Agent command-state wiring while
# continuing to reject every other runtime manifest and all write transports.
agent_sources = AGENT_SOURCES.read_text(encoding="utf-8") if AGENT_SOURCES.is_file() else ""
if agent_sources.count("core/agent/src/BackendAgentNativeTimerDelete.cpp") != 1:
    failures.append("Slice-29 Agent state owner must wire the Timer-delete domain validator exactly once")
if "AGENT_COMMAND_STATE_SRC :=" not in agent_sources:
    failures.append("Slice-29 Agent state owner command-state source set is missing")
# The future DELETE control plane owns a *second* explicitly checked
# consumer of the pure domain validator. The Agent state owner remains the
# only local executor; the daemon may not import direct SVDRP mutation.
delete_runner = ROOT / "core/daemon/src/NativeTimerDeleteProductiveRuntime.cpp"
delete_result = ROOT / "core/daemon/src/NativeTimerDeleteResultOutcomeApplication.cpp"
runner_text = delete_runner.read_text(encoding="utf-8") if delete_runner.is_file() else ""
result_text = delete_result.read_text(encoding="utf-8") if delete_result.is_file() else ""
delete_successor_valid = all(marker in runner_text for marker in (
    "publicNativeTimerDeletePreflight(",
    "applyDurableNativeTimerDeleteResult(",
    "verification.verify(",
    "completion.complete(",
    "BackendAgentNativeTimerDeleteAssignmentService& assigner",
)) and all(marker in result_text for marker in (
    "backendAgentNativeTimerDeleteParseResultEvidence(",
    "dispatchService.applyOutcome(",
    "NativeTimerBindingOwnership::managed",
    "NativeTimerBindingOwnership::adopted",
))
for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "BackendAgentNativeTimerDelete.cpp" in text and source_manifest != AGENT_SOURCES:
        if (
            source_manifest.name == "daemon-sources.mk"
            and delete_successor_valid
            and "NativeTimerDeleteProductiveRuntime.cpp" in text
            and "NativeTimerDeleteResultOutcomeApplication.cpp" in text
            and text.count("core/agent/src/BackendAgentNativeTimerDelete.cpp") == 1
        ):
            continue
        failures.append(
            f"Slice-24 contract wired outside verified DELETE successor: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 native Timer delete Agent contract check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer delete Agent contract check passed")
