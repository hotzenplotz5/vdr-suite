#!/usr/bin/env python3
"""Architecture guard for Phase 64 Slice 25 Timer-delete assignment persistence."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD = ROOT / "core/agent/include/BackendAgentNativeTimerDeletePayload.h"
HEADER = ROOT / "core/agent/include/BackendAgentNativeTimerDeleteAssignment.h"
SOURCE = ROOT / "core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp"
COMMAND = ROOT / "core/agent/src/BackendAgentCommand.cpp"
DELIVERY = ROOT / "core/agent/src/BackendAgentCommandDelivery.cpp"
CLIENT = ROOT / "core/agent/src/BackendAgentCommandClient.cpp"
TEST = ROOT / "core/agent/tests/test_backend_agent_native_timer_delete_assignment.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-delete-assignment.md"
FRAGMENT = ROOT / "mk/phase64-native-timer-delete-assignment-tests.mk"
MAKEFILE = ROOT / "Makefile"
AGENT_SOURCES = ROOT / "mk/agent-sources.mk"

failures: list[str] = []


def require(path: Path, markers: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing Slice-25 file: {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            failures.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")


require(PAYLOAD, [
    "BackendAgentNativeTimerDeletePayload",
    "timerDeleteSchema",
    "operationRevision",
    "expectedBindingRevision",
    "expectedNativeTimerFingerprint",
    "controlPlaneClaimedAt",
    "ownershipGeneration",
    "providerInstanceEpoch",
    "requiredCapability",
])
require(HEADER, [
    "BackendAgentNativeTimerDeleteAssignmentRequest",
    "BackendAgentNativeTimerDeleteAssignmentResult",
    "BackendAgentNativeTimerDeleteAssignmentService",
    "expectedNativeTimerFingerprint",
    "backendGeneration",
    "controlPlaneClaimedAt",
])
require(SOURCE, [
    "ensureNativeTimerDeleteAssignmentSchema",
    "idx_backend_agent_timer_delete_operation",
    "trg_backend_agent_timer_delete_dormant_capability",
    "DELETE FROM backend_agent_command_capabilities",
    "findAssignmentForOperation",
    "localProviderSelectionForCommand",
    "selectLocalProvider",
    "request.expectedNativeTimerFingerprint",
    "payload.expectedNativeTimerFingerprint",
    "kBackendAgentNativeTimerDeleteAuthorityDomain",
    "kBackendAgentNativeTimerDeleteCapability",
    "insertAssignment",
    "native_timer_delete_assignment_replayed",
    "native_timer_delete_provider_selection_stale",
])
require(COMMAND, [
    'BackendAgentNativeTimerDeletePayload.h',
    "kBackendAgentNativeTimerDeleteCommandType",
    "backendAgentNativeTimerDeleteParsePayload",
    "payload.controlPlaneClaimedAt <= value.assignedAt",
])
require(TEST, [
    'expectedNativeTimerFingerprint = "sha256:native-timer-observed-44"',
    "changedFingerprintRequest.expectedNativeTimerFingerprint",
    'supportedCommandTypes = {"probe.noop"}',
    'supportedCommandTypes = {"vdr.timer.delete"}',
    "blockedDelivery",
    'assert(!commands.hasCapability(',
    '"vdr.timer.delete"',
    "native_timer_delete_assignment_replayed",
    "native_timer_delete_provider_selection_stale",
    "local_provider_capability_not_observed",
    "assert(!notDelivered.assignment.present)",
])
require(DOC, [
    "Phase 64 Slice 25",
    "PR #177",
    "vdr.timer.delete",
    "command + provider selection",
    "dormant",
    "not advertised",
    "Slice 26",
])
require(FRAGMENT, [
    "test-phase64-native-timer-delete-assignment",
    "BackendAgentNativeTimerDeleteAssignment.cpp",
    "test_backend_agent_native_timer_delete_assignment.cpp",
    "test-fast: test-phase64-native-timer-delete-assignment",
])
require(MAKEFILE, ["include mk/phase64-native-timer-delete-assignment-tests.mk"])

source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
for forbidden in [
    "BackendAgentCommandClient",
    "SuiteBridgeCommandReply",
    "ISuiteBridgeLocalTransport",
    "SVDRP",
    "RESTfulAPI",
    "executeNative",
    "system(",
    "popen(",
    "hasCapability(",
]:
    if forbidden in source:
        failures.append(f"Slice-25 assignment contains forbidden delivery/write coupling: {forbidden}")

if DELIVERY.is_file() and "vdr.timer.delete" in DELIVERY.read_text(encoding="utf-8"):
    failures.append("Slice-25 unexpectedly changes Timer-delete delivery runtime")
if CLIENT.is_file() and "vdr.timer.delete" in CLIENT.read_text(encoding="utf-8"):
    failures.append("Slice-25 unexpectedly advertises/executes Timer delete in Agent client")

# The assignment repository remains control-plane-only. Slice 29 may wire the
# pure Slice-24 command validator into the bounded Agent state-owner source set,
# but the Slice-25 assignment persistence source must not enter runtime there.
agent_sources = AGENT_SOURCES.read_text(encoding="utf-8") if AGENT_SOURCES.is_file() else ""
if agent_sources.count("core/agent/src/BackendAgentNativeTimerDelete.cpp") != 1:
    failures.append("Slice-29 Agent state owner must wire the Timer-delete domain validator exactly once")
if "AGENT_COMMAND_STATE_SRC :=" not in agent_sources:
    failures.append("Slice-29 Agent state owner command-state source set is missing")
for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "BackendAgentNativeTimerDeleteAssignment.cpp" in text:
        failures.append(
            f"Slice-25 assignment unexpectedly wired into runtime sources: {source_manifest.relative_to(ROOT)}"
        )
    if "BackendAgentNativeTimerDelete.cpp" in text and source_manifest != AGENT_SOURCES:
        failures.append(
            f"Slice-24 Timer-delete contract unexpectedly wired outside the bounded Agent state owner: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 native Timer delete assignment check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer delete assignment check passed")
