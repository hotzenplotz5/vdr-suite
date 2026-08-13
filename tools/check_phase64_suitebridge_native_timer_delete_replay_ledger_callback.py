#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required replay-ledger file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
header = read("vdr-plugin-suite-bridge/suitebridge_native_timer_delete.h")
source = read("vdr-plugin-suite-bridge/suitebridge_native_timer_delete.cpp")
test = read("vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_delete.cpp")
plugin_main = read("vdr-plugin-suite-bridge/suitebridge.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")
mk = read("mk/phase64-suitebridge-native-timer-delete-replay-ledger-callback-tests.mk")
doc = read(
    "docs/development/phase-64-suitebridge-native-timer-delete-replay-ledger-callback.md"
)

include = "include mk/phase64-suitebridge-native-timer-delete-replay-ledger-callback-tests.mk"
require(makefile, include, "replay-ledger Make include")
if makefile.count(include) != 1:
    raise SystemExit("replay-ledger Make include must occur exactly once")

for needle, label in (
    ("SuiteBridgeNativeTimerDeleteRequest", "typed mutation request"),
    ("SuiteBridgeNativeTimerDeleteMutationDisposition", "typed mutation disposition"),
    ("ISuiteBridgeNativeTimerDeleteMutationCallback", "typed mutation callback"),
    ("std::mutex replayMutex_", "replay ledger mutex"),
    ("replayByOperationId_", "operation-scoped replay ledger"),
    ("operationByCommandId_", "command-to-operation fence"),
    ("maximumReplayEntries_", "bounded replay ledger"),
):
    require(header, needle, label)

for needle, label in (
    ("canonicalRequest(request)", "exact semantic request canonicalization"),
    ("request.operationId", "operation identity binding"),
    ("request.requestFingerprint", "request fingerprint binding"),
    ("operationByCommandId_.find(request.commandId)", "command identity conflict check"),
    ("replayByOperationId_.find(request.operationId)", "operation replay lookup"),
    ("entry.canonicalRequest != canonical", "exact request replay conflict"),
    ("replayByOperationId_.size() >= maximumReplayEntries_", "fail-closed ledger capacity"),
    ("replayByOperationId_.emplace(request.operationId", "reservation before callback"),
    ("mutationCallback_->DeleteTimer(request)", "typed callback invocation"),
    ("existing->second.terminal = true", "terminal replay persistence"),
    ("OutcomeUnknown", "ambiguous callback outcome"),
    ("callback-exception", "callback exception ambiguity"),
    ("replay_conflict", "idempotency conflict result"),
    ("ledger_full", "bounded-ledger no-effect result"),
    ("in_progress", "reserved duplicate result"),
):
    require(source, needle, label)

reserve_pos = source.find("replayByOperationId_.emplace(request.operationId")
callback_pos = source.find("mutationCallback_->DeleteTimer(request)")
if reserve_pos < 0 or callback_pos < 0 or reserve_pos >= callback_pos:
    raise SystemExit("callback side effect must occur only after replay reservation")

for token in (
    "<vdr/timers.h>",
    "cTimers",
    "Timers->",
    '"DELT"',
    "RESTfulAPI",
    "restfulapi",
):
    forbid(source, token, "real VDR mutation in replay-ledger slice")

require(
    plugin_main,
    "nativeTimerDelete_(nativeProbe_.PluginInstanceEpoch())",
    "production callback remains unconfigured",
)
forbid(plugin_main, "ISuiteBridgeNativeTimerDeleteMutationCallback", "production callback wiring")
forbid(agent_client, "SuiteBridgeNativeTimerDeleteTransport", "installed Agent Timer-delete transport wiring")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete advertisement")

for needle, label in (
    ("callback.calls == 1", "exactly-once callback assertions"),
    ("reentrantReply.replyCode == 558", "in-progress replay regression"),
    ("conflict.replyCode == 559", "fingerprint conflict regression"),
    ("full.replyCode == 560", "ledger capacity regression"),
    ("callback.throwOnCall = true", "exception ambiguity regression"),
    ("replay.payload == first.payload", "terminal replay result regression"),
):
    require(test, needle, label)

require(mk, "test-phase64-suitebridge-native-timer-delete-disabled-transport", "Slice 34 dependency")
require(
    mk,
    "check_phase64_suitebridge_native_timer_delete_replay_ledger_callback.py",
    "replay-ledger guard invocation",
)

for needle, label in (
    ("reserve-before-side-effect", "reserve-before-side-effect documentation"),
    ("plugin-instance-scoped", "plugin-instance replay scope documentation"),
    ("no real VDR Timer mutation", "no-mutation documentation"),
    ("production plugin does not configure", "production callback fence documentation"),
    ("durable Control Plane", "durable idempotency layering documentation"),
):
    require(doc, needle, label)

print("Phase 64 SuiteBridge Timer-delete replay ledger/callback architecture guard passed")