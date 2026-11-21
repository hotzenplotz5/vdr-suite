#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 27 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
header = read("core/agent/include/BackendAgentNativeTimerDeleteLocalState.h")
source = read("core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp")
test = read("core/agent/tests/test_backend_agent_native_timer_delete_local_state.cpp")
doc = read("docs/development/phase-64-native-timer-delete-local-starting.md")
mk = read("mk/phase64-native-timer-delete-local-starting-tests.mk")
agent_sources = read("mk/agent-sources.mk")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")

include = "include mk/phase64-native-timer-delete-local-starting-tests.mk"
require(makefile, include, "Slice 27 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 27 Make include must occur exactly once")

require(
    header,
    "BackendAgentNativeTimerDeleteRecoveryDecision",
    "typed Timer-delete recovery decision",
)
require(
    header,
    "reconcileOnly",
    "reconcile-only recovery state",
)
require(
    source,
    "native_timer_delete_starting_recovery_reconcile_only",
    "same-context crash recovery fence",
)
require(
    source,
    "native_timer_delete_starting_context_fenced_reconcile_only",
    "generation/context drift recovery fence",
)
require(
    source,
    "local-recovery:",
    "durable-starting outcome-unknown evidence reference",
)
require(
    source,
    "BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown",
    "outcome-unknown recovery evidence",
)
require(
    source,
    "returnPersistedEvidence",
    "completed evidence replay",
)
require(
    source,
    "backendAgentNativeTimerDeleteParsePayload",
    "strict Slice 25 payload reuse",
)
require(
    source,
    "candidate.expectedNativeTimerFingerprint = payload.expectedNativeTimerFingerprint",
    "immutable native Timer fingerprint assignment mapping",
)
require(
    source,
    '"expected_native_timer_fingerprint"',
    "durable native Timer fingerprint local-state key",
)
require(
    source,
    "command.expectedNativeTimerFingerprint",
    "native Timer fingerprint local-state serialization",
)
require(
    source,
    'values["expected_native_timer_fingerprint"]',
    "native Timer fingerprint local-state parse",
)
require(
    source,
    "backendAgentNativeTimerDeleteEvidenceMatches",
    "Slice 24 evidence chronology reuse",
)
require(
    test,
    "every recovery path is reconciliation-only",
    "no-blind-retry regression",
)
require(
    test,
    '"expected_native_timer_fingerprint=" + timerFingerprint()',
    "SHA-256 native Timer fingerprint persistence regression",
)
require(
    test,
    "parsedStarting.command.expectedNativeTimerFingerprint",
    "native Timer fingerprint starting replay regression",
)
require(
    test,
    "missingFingerprint",
    "missing native Timer fingerprint fail-closed regression",
)
require(
    test,
    "completed_evidence_survives_context_drift",
    "post-completion context drift regression",
)
require(
    doc,
    "Hazard window",
    "durable-starting hazard boundary documentation",
)
require(
    doc,
    "No runtime wiring",
    "non-runtime scope boundary documentation",
)
require(
    mk,
    "test-phase64-native-timer-delete-local-starting",
    "focused Slice 27 target",
)

# Slice 27 remains a typed local-state contract. The v3 state-owner successor
# may link that contract for validation/persistence, but it still cannot expose
# a Timer-delete command or any side-effectful transport.
require(
    agent_sources,
    "core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp",
    "v3 state-owner local-state validation wiring",
)
forbid(command_client, "vdr.timer.delete", "Timer-delete command-client execution")
require(agent_client, "kBackendAgentNativeTimerDeleteCommandType",
        "Timer-delete Agent configuration allowlist")
require(packaged_config, "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete", "accepted packaged Timer activation")

for token in (
    "SuiteBridgeSvdrp",
    "ISuiteBridgeLocalTransport",
    "restfulapi",
    "RESTfulAPI",
    "SVDRP",
    "system(",
    "popen(",
    "curl ",
    "/timers",
):
    forbid(source, token, "native mutation/transport coupling in local-state contract")

print("Phase 64 native Timer delete local starting architecture guard passed")
