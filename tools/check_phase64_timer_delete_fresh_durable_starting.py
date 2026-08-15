#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 31 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")
test = read("core/agent/tests/test_backend_agent_timer_delete_fresh_durable_starting.cpp")
mk = read("mk/phase64-timer-delete-fresh-durable-starting-tests.mk")
doc = read("docs/development/phase-64-timer-delete-fresh-durable-starting.md")
slice30_guard = read("tools/check_phase64_timer_delete_local_state_lifecycle.py")

include = "include mk/phase64-timer-delete-fresh-durable-starting-tests.mk"
require(makefile, include, "Slice 31 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 31 Make include must occur exactly once")

# The fresh handoff reuses the Slice-27 typed state contract and the Slice-28
# generic extension. The existing protected v3 writer remains the sole durable
# state owner.
require(
    command_client,
    "prepareFreshNativeTimerDeleteLocalStarting",
    "bounded fresh starting helper",
)
require(
    command_client,
    "backendAgentNativeTimerDeletePrepareLocalStarting",
    "typed fresh starting preparation",
)
require(
    command_client,
    "backendAgentNativeTimerDeleteSerializeLocalState",
    "typed starting serialization",
)
require(
    command_client,
    "backendAgentCommandStateExtensionValidateSupported",
    "typed extension validation before persist",
)
require(
    command_client,
    'state.dispatchState = "starting"',
    "generic starting projection",
)

fresh_helper = command_client.split(
    "bool prepareFreshNativeTimerDeleteLocalStarting(", 1
)[1].split("\nbool negotiateNativeCapability(", 1)[0]
require(fresh_helper, "persist(config.statePath, state, reason)", "durable starting write")
forbid(fresh_helper, "sendReceipt(", "receipt before fresh helper returns")
forbid(fresh_helper, "sendResult(", "result emission from fresh helper")

# Ordering is the core safety property. Existing typed evidence is recovered
# before context drift can hide it. A fresh state is instead armed only after
# current-context and deadline checks, and the accepted receipt follows the
# durable starting write.
reconcile = command_client.split(
    "bool reconcileBackendAgentCommandState(", 1
)[1].split("\nbool pollBackendAgentCommand(", 1)[0]
recover = reconcile.find("reconcileNativeTimerDeleteLocalState(")
context = reconcile.find("if (!sameContext(state.assignment, context))")
deadline = reconcile.find("state.assignment.deadline <= currentTime")
fresh = reconcile.find("prepareFreshNativeTimerDeleteLocalStarting(")
receipt = reconcile.find("sendReceipt(config, context, transport, state, reason)", fresh)
if min(recover, context, deadline, fresh, receipt) < 0:
    raise SystemExit("missing Slice 31 reconciliation ordering marker")
if not recover < context < deadline < fresh < receipt:
    raise SystemExit(
        "required ordering is recovery -> context fence -> deadline -> durable starting -> receipt"
    )
require(
    reconcile,
    "timerDeleteCommand && state.stateExtensionPresent",
    "existing-evidence recovery discriminator",
)
require(
    reconcile,
    "timerDeleteCommand && !state.stateExtensionPresent && !state.resultPresent",
    "fresh-state discriminator",
)
require(
    reconcile,
    'reason = "native_delete_local_starting_handoff_persisted"',
    "fresh handoff completion boundary",
)
require(
    reconcile,
    'reason = "local_command_generation_fenced"',
    "fresh generation fence",
)
require(
    reconcile,
    '"expired"',
    "fresh deadline rejection",
)

# The previous Slice-30 guard is successor-aware rather than silently losing
# its recovery assertions when fresh starting becomes legal in Slice 31.
require(
    slice30_guard,
    "bounded successor fresh-starting handoff",
    "Slice 30 successor allowance",
)

# There is still no Timer-delete availability or execution path. The type is
# not advertised, cannot be configured, and has no native write transport.
available = command_client.split("CommandAvailability availableCommands(", 1)[1].split(
    "\n}\n}\n\nbool reconcileBackendAgentCommandState(", 1
)[0]
require(
    available,
    "kBackendAgentNativeTimerDeleteCommandType",
    "Timer-delete availability fence",
)
require(available, "continue;", "Timer-delete advertisement suppression")
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

# Focused regression locks the new boundary and its failure modes.
for needle, label in (
    ("Fresh current-context handoff persists typed starting before the accepted", "fresh starting before receipt"),
    ("never performs a second fresh preparation or blind retry", "no-blind-retry successor recovery"),
    ("Fresh starting is fenced by the current Agent/backend generation", "generation-fenced fresh state"),
    ("Expiry is checked before fresh starting", "deadline-before-starting regression"),
    ("Receipt transport failure after the durable starting write", "lost receipt after starting regression"),
    ("COMMAND_STATE", "placeholder"),
):
    if label == "placeholder":
        continue
    require(test, needle, label)

require(
    test,
    "BackendAgentNativeTimerDeleteLocalPhase::starting",
    "typed starting persistence assertion",
)
require(
    test,
    "BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown",
    "recovery outcome assertion",
)
require(
    mk,
    "test-phase64-timer-delete-local-state-lifecycle",
    "Slice 30 regression dependency",
)
require(doc, "accepted receipt", "receipt ordering documentation")
require(doc, "No execution boundary", "non-execution architecture boundary")
require(doc, "outcome_unknown", "recovery semantics documentation")

print("Phase 64 Timer delete fresh durable starting architecture guard passed")
