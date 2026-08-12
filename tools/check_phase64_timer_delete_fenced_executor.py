#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 32 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
agent_sources = read("mk/agent-sources.mk")
header = read("core/agent/include/BackendAgentNativeTimerDeleteExecutor.h")
source = read("core/agent/src/BackendAgentNativeTimerDeleteExecutor.cpp")
test = read("core/agent/tests/test_backend_agent_timer_delete_fenced_executor.cpp")
mk = read("mk/phase64-timer-delete-fenced-executor-tests.mk")
doc = read("docs/development/phase-64-timer-delete-fenced-executor.md")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
transport_header = read("core/agent/include/SuiteBridgeSvdrpTransport.h")
transport_source = read("core/agent/src/SuiteBridgeSvdrpTransport.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")

include = "include mk/phase64-timer-delete-fenced-executor-tests.mk"
require(makefile, include, "Slice 32 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 32 Make include must occur exactly once")

require(
    agent_sources,
    "AGENT_TIMER_DELETE_EXECUTOR_SRC :=",
    "separate executor source set",
)
executor_source = "core/agent/src/BackendAgentNativeTimerDeleteExecutor.cpp"
if agent_sources.count(executor_source) != 1:
    raise SystemExit("Timer-delete executor source must occur exactly once")
executor_block = agent_sources.split(
    "AGENT_TIMER_DELETE_EXECUTOR_SRC :=", 1
)[1].split("\n\nAGENT_COMMAND_CLIENT_SRC", 1)[0]
require(executor_block, executor_source, "executor source ownership")
for token in ("SuiteBridge", "Svdrp", "REST", "restful"):
    forbid(executor_block, token, "concrete transport in executor source set")
require(
    header,
    "class IBackendAgentNativeTimerDeleteTransport",
    "typed Timer-delete transport interface",
)
require(
    header,
    "BackendAgentNativeTimerDeleteTransportDisposition",
    "bounded transport disposition",
)
require(
    header,
    "backendAgentNativeTimerDeleteExecuteFreshStartingOnce",
    "fresh one-shot executor API",
)

# The persisted selection is authority. Current provider facts only prove that
# exactly the selected provider instance/generation/capability revision remains
# usable immediately before dispatch.
for needle, label in (
    ("backendAgentLocalProviderValidSelection(selection)", "persisted selection validation"),
    ("backendAgentLocalProviderValidFacts(facts)", "provider facts validation"),
    ("selection.providerId == facts.providerId", "provider id fence"),
    ("selection.providerKind == facts.providerKind", "provider kind fence"),
    ("selection.providerInstanceEpoch == facts.providerInstanceEpoch", "provider epoch fence"),
    ("selection.providerGeneration == facts.providerGeneration", "provider generation fence"),
    ("selection.capabilityRevision == facts.capabilityRevision", "capability revision fence"),
    ("selection.requiredCapability", "required capability fence"),
    ("facts.available", "provider availability fence"),
):
    require(source, needle, label)

require(source, "assignment.deadline <= context.now", "deadline recheck")
require(source, "!sameContext(localState.command, context)", "Agent/generation fence")
require(
    source,
    "localState.phase != BackendAgentNativeTimerDeleteLocalPhase::starting",
    "fresh starting phase requirement",
)
require(
    source,
    "backendAgentNativeTimerDeleteCommandFromAssignment",
    "assignment-to-local-state command reconstruction",
)

# There is one and only one effectful injected transport call and no internal
# retry loop. Discovery is pre-dispatch and cannot mutate a VDR timer.
delete_call = "transport.deleteTimer(request)"
if source.count(delete_call) != 1:
    raise SystemExit("Timer-delete executor must contain exactly one deleteTimer call")
for token in ("while (", "while(", "for (", "for("):
    forbid(source, token, "executor retry/iteration loop")

for outcome in (
    "rejectedWithoutEffect",
    "acceptedUnverified",
    "outcomeUnknown",
):
    require(source, outcome, f"{outcome} executor outcome")

require(source, "catch (...)", "ambiguous dispatch exception handling")
require(
    source,
    '"executor:dispatch-exception"',
    "post-dispatch exception outcome evidence",
)
require(
    source,
    '"executor:accepted-evidence-invalid"',
    "invalid accepted evidence downgrade",
)

# Slice 32 is contract-only. It must not become reachable from the production
# command owner or gain a concrete SuiteBridge/VDR mutation transport.
for token in (
    "BackendAgentNativeTimerDeleteExecutor",
    "IBackendAgentNativeTimerDeleteTransport",
    "backendAgentNativeTimerDeleteExecuteFreshStartingOnce",
):
    forbid(command_client, token, "Slice 32 CommandClient runtime wiring")

for text, label in (
    (transport_header, "SuiteBridge transport header"),
    (transport_source, "SuiteBridge transport source"),
):
    forbid(
        text,
        "IBackendAgentNativeTimerDeleteTransport",
        f"{label} Timer-delete implementation",
    )
    forbid(
        text,
        "vdr.timer.delete",
        f"{label} Timer-delete command",
    )

for token in (
    '"DELT"',
    "cTimers",
    "Timers->",
    "RESTfulAPI",
    "restfulapi",
    "SVDRP",
    "system(",
    "popen(",
    "curl ",
    "/timers",
):
    forbid(source, token, "concrete Timer-delete mutation coupling")

forbid(agent_client, "vdr.timer.delete", "Timer-delete Agent configuration")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete configuration")
require(
    command_client,
    "kBackendAgentNativeTimerDeleteCommandType",
    "existing Timer-delete advertisement fence",
)

# Focused tests cover pre-dispatch fences and every ambiguous post-dispatch
# category without a real SuiteBridge/VDR mutation transport.
for needle, label in (
    ("Exact persisted selection + current facts allows exactly one typed", "accepted one-shot path"),
    ("Agent/backend-generation drift is a definitive pre-dispatch fence", "generation fence regression"),
    ("Deadline is checked again immediately before provider discovery", "deadline fence regression"),
    ("Provider availability never creates authority", "provider authority regression"),
    ("the executor still never retries it", "no-retry transport rejection"),
    ("exceptions, explicit ambiguity, or malformed", "ambiguous outcome regression"),
    ("Completed local state is historical evidence", "completed evidence non-authority regression"),
):
    require(test, needle, label)

require(
    mk,
    "test-phase64-timer-delete-fresh-durable-starting",
    "Slice 31 regression dependency",
)
require(
    mk,
    "$(AGENT_TIMER_DELETE_EXECUTOR_SRC)",
    "dedicated executor build source",
)
require(doc, "Availability cannot replace", "authority-vs-availability documentation")
require(doc, "exactly one `deleteTimer()` call", "single dispatch documentation")
require(doc, "does **not** wire", "non-runtime-wiring documentation")
require(doc, "authoritative native-timer absence readback", "later readback boundary")

print("Phase 64 Timer delete fenced executor architecture guard passed")
