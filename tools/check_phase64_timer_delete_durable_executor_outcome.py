#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 33 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
client_header = read("core/agent/include/BackendAgentCommandClient.h")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
executor_source = read("core/agent/src/BackendAgentNativeTimerDeleteExecutor.cpp")
test = read("core/agent/tests/test_backend_agent_timer_delete_durable_executor_outcome.cpp")
mk = read("mk/phase64-timer-delete-durable-executor-outcome-tests.mk")
doc = read("docs/development/phase-64-timer-delete-durable-executor-outcome.md")
slice32_guard = read("tools/check_phase64_timer_delete_fenced_executor.py")
transport_header = read("core/agent/include/SuiteBridgeSvdrpTransport.h")
transport_source = read("core/agent/src/SuiteBridgeSvdrpTransport.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")

include = "include mk/phase64-timer-delete-durable-executor-outcome-tests.mk"
require(makefile, include, "Slice 33 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 33 Make include must occur exactly once")

# The executor transport is per-config injection only. There is deliberately no
# global setter or runtime discovery path for Timer-delete in this slice.
require(
    client_header,
    "class IBackendAgentNativeTimerDeleteTransport;",
    "typed Timer-delete transport forward declaration",
)
require(
    client_header,
    "IBackendAgentNativeTimerDeleteTransport* nativeTimerDeleteTransport = nullptr;",
    "default-disabled Timer-delete transport injection",
)
forbid(
    client_header,
    "setBackendAgentNativeTimerDeleteTransport",
    "global Timer-delete transport setter",
)
require(
    command_client,
    '#include "BackendAgentNativeTimerDeleteExecutor.h"',
    "Timer-delete executor dependency",
)

# The bounded completion helper consumes only a fresh, receipt-acknowledged
# starting state, invokes the one-shot executor once, and durably stores typed
# completed evidence plus its generic result before any result transmission.
helper_name = "executeFreshNativeTimerDeleteAndPersistOutcome"
if command_client.count(helper_name + "(") != 2:
    raise SystemExit("Slice 33 executor handoff must have one definition and one call")
helper = command_client.split(
    "bool executeFreshNativeTimerDeleteAndPersistOutcome(", 1
)[1].split("\nbool negotiateNativeCapability(", 1)[0]
for needle, label in (
    ("config.nativeTimerDeleteTransport == nullptr", "null transport fence"),
    ("!state.receiptAcknowledged", "receipt acknowledgement prerequisite"),
    ("backendAgentNativeTimerDeleteParseLocalState", "typed starting decode"),
    ("BackendAgentNativeTimerDeleteLocalPhase::starting", "fresh starting phase fence"),
    ("BackendAgentNativeTimerDeleteExecutorContext", "current executor context"),
    ("executorContext.backendGeneration = context.backendGeneration", "generation handoff"),
    ("executorContext.now = nowSeconds()", "dispatch-time clock fence"),
    ("backendAgentNativeTimerDeleteExecuteFreshStartingOnce", "one-shot executor call"),
    ("backendAgentNativeTimerDeleteCompleteLocalState", "typed completion"),
    ("backendAgentNativeTimerDeleteSerializeLocalState", "typed completed serialization"),
    ("nativeTimerDeleteGenericProjection", "generic outcome projection"),
    ("state.result.completedAt = evidence.completedAt", "shared evidence timestamp"),
    ("persist(config.statePath, state, reason)", "durable completed state/result write"),
):
    require(helper, needle, label)
if helper.count("backendAgentNativeTimerDeleteExecuteFreshStartingOnce(") != 1:
    raise SystemExit("Slice 33 completion helper must invoke executor exactly once")
forbid(helper, "sendReceipt(", "control-plane receipt from executor helper")
forbid(helper, "sendResult(", "control-plane result before durable outcome return")

# Existing-state recovery remains before current-context checks. Fresh execution
# exists only in the same invocation that created the new durable starting state.
reconcile = command_client.split(
    "bool reconcileBackendAgentCommandState(", 1
)[1].split("\nbool pollBackendAgentCommand(", 1)[0]
recover = reconcile.find("reconcileNativeTimerDeleteLocalState(")
context = reconcile.find("if (!sameContext(state.assignment, context))")
deadline = reconcile.find("state.assignment.deadline <= currentTime")
fresh = reconcile.find("prepareFreshNativeTimerDeleteLocalStarting(")
receipt = reconcile.find("sendReceipt(config, context, transport, state, reason)", fresh)
disabled = reconcile.find("config.nativeTimerDeleteTransport == nullptr", receipt)
execute = reconcile.find("executeFreshNativeTimerDeleteAndPersistOutcome(", disabled)
result = reconcile.find("sendResult(config, context, transport, state, reason)", execute)
if min(recover, context, deadline, fresh, receipt, disabled, execute, result) < 0:
    raise SystemExit("missing Slice 33 fresh executor ordering marker")
if not recover < context < deadline < fresh < receipt < disabled < execute < result:
    raise SystemExit(
        "required ordering is recovery -> context -> deadline -> durable starting -> receipt -> disabled fence -> executor outcome persist -> result"
    )
require(
    reconcile,
    'reason = "native_delete_local_starting_handoff_persisted"',
    "default-disabled Slice-31 handoff",
)
require(
    reconcile,
    'reason = "native_delete_executor_outcome_reconciled"',
    "fresh executor outcome completion",
)

# Slice 32 must explicitly acknowledge this bounded successor rather than losing
# its no-concrete-transport assertions.
require(
    slice32_guard,
    "bounded Slice 33 durable-outcome successor",
    "Slice 32 successor allowance",
)

# The executor contract itself remains one-shot and provider-fenced.
if executor_source.count("transport.deleteTimer(request)") != 1:
    raise SystemExit("Slice 32 executor must still contain exactly one deleteTimer call")
for needle in (
    "selection.providerInstanceEpoch == facts.providerInstanceEpoch",
    "selection.providerGeneration == facts.providerGeneration",
    "selection.capabilityRevision == facts.capabilityRevision",
    "facts.available",
):
    require(executor_source, needle, "preserved provider fence")

# Still no concrete mutation transport or normal availability/configuration.
for text, label in (
    (transport_header, "SuiteBridge transport header"),
    (transport_source, "SuiteBridge transport source"),
):
    forbid(text, "IBackendAgentNativeTimerDeleteTransport", f"{label} Timer-delete implementation")
    forbid(text, "vdr.timer.delete", f"{label} Timer-delete command")

for token in (
    '"DELT"',
    "cTimers",
    "Timers->",
    "RESTfulAPI",
    "restfulapi",
    "system(",
    "popen(",
    "curl ",
    "/timers",
):
    forbid(command_client, token, "concrete Timer-delete mutation coupling")

forbid(agent_client, "vdr.timer.delete", "Timer-delete Agent configuration")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete configuration")
available = command_client.split("CommandAvailability availableCommands(", 1)[1].split(
    "\n}\n}\n\nbool reconcileBackendAgentCommandState(", 1
)[0]
require(available, "kBackendAgentNativeTimerDeleteCommandType", "Timer-delete availability fence")
require(available, "continue;", "Timer-delete advertisement suppression")

# Focused integration regressions lock the no-blind-retry and evidence-ordering
# semantics without a real SuiteBridge or VDR mutation transport.
for needle, label in (
    ("With no injected Timer-delete transport", "default-disabled path"),
    ("Fresh durable starting -> accepted receipt -> exactly one executor call", "fresh executor ordering"),
    ("Completed evidence is replay authority", "completed evidence no-reexecution"),
    ("Provider drift is decided before dispatch", "pre-dispatch provider fence"),
    ("ambiguous post-dispatch exception becomes durable outcome_unknown", "ambiguous outcome durability"),
    ("Result transport can fail only after completed evidence", "durable-before-result replay"),
    ("Receipt transport loss occurs before executor dispatch", "receipt-loss no blind retry"),
):
    require(test, needle, label)

require(mk, "test-phase64-timer-delete-fenced-executor", "Slice 32 regression dependency")
require(doc, "receipt acknowledgement", "receipt-before-executor documentation")
require(doc, "never repeated", "no-blind-retry documentation")
require(doc, "no concrete Timer-delete mutation transport", "non-production-mutation boundary")
require(doc, "authoritative native-timer absence readback", "later readback boundary")

print("Phase 64 Timer delete durable executor outcome architecture guard passed")
