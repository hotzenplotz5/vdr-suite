#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 30 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
test = read("core/agent/tests/test_backend_agent_command_state_v3.cpp")
mk = read("mk/phase64-timer-delete-local-state-lifecycle-tests.mk")
doc = read("docs/development/phase-64-timer-delete-local-state-lifecycle.md")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")

include = "include mk/phase64-timer-delete-local-state-lifecycle-tests.mk"
require(makefile, include, "Slice 30 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 30 Make include must occur exactly once")

# Slice 30 consumes only already-durable typed local state. It must never open
# a path that prepares a fresh starting record or enters an executor.
require(
    command_client,
    '#include "BackendAgentNativeTimerDeleteLocalState.h"',
    "typed Timer-delete lifecycle dependency",
)
require(
    command_client,
    "backendAgentNativeTimerDeleteRecoverLocalState",
    "typed recovery decision",
)
require(
    command_client,
    "backendAgentNativeTimerDeleteCompleteLocalState",
    "durable starting-to-completed conversion",
)
require(
    command_client,
    "backendAgentNativeTimerDeleteSerializeLocalState",
    "completed typed-state persistence",
)
require(
    command_client,
    "BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect",
    "rejected-without-effect projection",
)
require(
    command_client,
    "BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified",
    "accepted-unverified projection",
)
require(
    command_client,
    "BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown",
    "outcome-unknown projection",
)
require(command_client, '"reconcile_only"', "reconciliation-only generic result")
forbid(
    command_client,
    "backendAgentNativeTimerDeletePrepareLocalStarting",
    "fresh Timer-delete starting preparation",
)
forbid(command_client, "vdr.timer.delete", "literal Timer-delete execution branch")

# Recovery must happen before the generic generation fence so completed
# evidence survives context drift, while the fence still blocks result send.
recover_call = command_client.find("reconcileNativeTimerDeleteLocalState(")
context_fence = command_client.find("if (!sameContext(state.assignment, context))")
if recover_call < 0 or context_fence < 0 or recover_call > context_fence:
    raise SystemExit("Timer-delete evidence recovery must precede generic context fence")
require(
    command_client,
    "local_command_generation_fenced",
    "post-recovery generation fence",
)

# Timer-delete remains unavailable to normal command polling and configuration.
require(
    command_client,
    "kBackendAgentNativeTimerDeleteCommandType",
    "Timer-delete type fence",
)
forbid(agent_client, "vdr.timer.delete", "Timer-delete Agent configuration")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete configuration")

for token in (
    "SuiteBridgeSvdrp",
    "ISuiteBridgeLocalTransport",
    "RESTfulAPI",
    "restfulapi",
    "SVDRP",
    '"DELT"',
    "system(",
    "popen(",
    "curl ",
    "/timers",
):
    forbid(command_client, token, "Timer-delete executor/write transport coupling")

# Focused regression proves all lifecycle projections and drift behavior while
# preserving the existing v1/v2/v3 and advertisement tests.
for needle, label in (
    ("starting recovery becomes durable completed outcome-unknown evidence", "starting recovery regression"),
    ("completed evidence survives context drift", "completed context-drift regression"),
    ("rejected-without-effect projects to a verified rejection", "rejected projection regression"),
    ("accepted-unverified remains reconciliation-only", "accepted-unverified projection regression"),
    ("assertTimerDeleteSuppressed", "advertisement suppression regression"),
):
    require(test, needle, label)

require(
    mk,
    "test-phase64-command-state-v3-extension",
    "Slice 29 state-owner regression dependency",
)
require(doc, "No execution boundary", "non-execution scope boundary")
require(doc, "outcome_unknown", "recovery outcome documentation")

print("Phase 64 Timer delete local-state lifecycle architecture guard passed")
