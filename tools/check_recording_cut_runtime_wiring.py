#!/usr/bin/env python3
"""Static Slice-3 guard for fenced native VDR recording cutting."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
errors: list[str] = []


def text(relative: str) -> str:
    path = ROOT / relative
    if not path.is_file():
        errors.append(f"missing required file: {relative}")
        return ""
    return path.read_text(encoding="utf-8")


agent_main = text("apps/agent/main.cpp")
client_header = text("core/agent/include/BackendAgentCommandClient.h")
client = text("core/agent/src/BackendAgentCommandClient.cpp")
domain = text("core/agent/src/BackendAgentRecordingCut.cpp")
payload = text("core/agent/src/BackendAgentRecordingCutPayload.cpp")
assignment = text("core/agent/src/BackendAgentRecordingCutAssignment.cpp")
local_state = text("core/agent/src/BackendAgentRecordingCutLocalState.cpp")
executor = text("core/agent/src/BackendAgentRecordingCutExecutor.cpp")
handler = text("core/agent/src/BackendAgentRecordingCutCommandHandler.cpp")
transport = text("core/agent/src/SuiteBridgeSvdrpRecordingCutTransport.cpp")
plugin_protocol = text("vdr-plugin-suite-bridge/suitebridge_recording_cut.cpp")
plugin_state = text("vdr-plugin-suite-bridge/suitebridge_recording_cut_state.h")
plugin_vdr = text("vdr-plugin-suite-bridge/suitebridge_recording_cut_vdr.cpp")
plugin_svdrp = text("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
daemon_match = text("core/daemon/src/DaemonRecordingCutReconciliation.cpp")
daemon_cut = text("core/daemon/src/DaemonRuntimeRecordingCut.cpp")
daemon_editing = text("core/daemon/src/DaemonRuntimeRecordingEditing.cpp")
daemon_runtime = text("core/daemon/src/DaemonRuntime.cpp")

for label, content, tokens in (
    ("shipped Agent", agent_main, (
        "SuiteBridgeRecordingCutTransport",
        "kBackendAgentRecordingCutCommandType",
        "setBackendAgentRecordingCutTransport(recordingCutTransport.get())",
    )),
    ("CommandClient header", client_header, (
        "RecordingCutDefaultTransport",
        "recordingCutTransport",
        "setBackendAgentRecordingCutTransport",
    )),
    ("CommandClient runtime", client, (
        "reconcileRecordingCutLocalState",
        "prepareFreshRecordingCutLocalStarting",
        "executeFreshRecordingCutAndPersistOutcome",
        "kBackendAgentRecordingCutCapability",
        "config.recordingCutTransport",
        "recording_cut_local_starting_handoff_persisted",
    )),
    ("cut domain", domain, (
        "kBackendAgentRecordingCutAuthorityDomain",
        "kBackendAgentRecordingCutProviderId",
        "kBackendAgentRecordingCutCapability",
        "expectedMarksRevision",
    )),
    ("cut payload", payload, (
        "expectedMarksRevision",
        "controlPlaneClaimedAt",
        "localProviderSelection",
    )),
    ("cut assignment", assignment, (
        "kBackendAgentRecordingCutCommandType",
        'assignment.verificationPolicy = "readback_required"',
        "selectLocalProvider",
    )),
    ("cut local state", local_state, (
        "recording_cut_starting_ready_for_durable_persist",
        "BackendAgentRecordingCutRecoveryDecision::reconcileOnly",
        "recording_cut_starting_recovery_reconcile_only",
        "outcomeUnknown",
    )),
    ("cut executor", executor, (
        "transport.startCut(request)",
        "outcomeUnknown",
        "backendAgentLocalProviderSameFence",
    )),
    ("cut command handler", handler, (
        "backendAgentRecordingCutCommandPrepareFreshStarting",
        "backendAgentRecordingCutCommandExecuteFreshStartingAndPersistOutcome",
        "backendAgentRecordingCutCommandReconcileExisting",
    )),
    ("typed cut transport", transport, (
        "NCUT",
        "kBackendAgentRecordingCutCapability",
        "outcomeUnknown",
        "providerInstanceEpoch",
    )),
    ("SuiteBridge cut protocol", plugin_protocol, (
        "vdr.recording.cut",
        "outcome_unknown",
        "replay_conflict",
    )),
    ("RCUT state contract", plugin_state, (
        "SuiteBridgeRecordingCutState",
        "vdr-suite-rcut-state/1",
        "editedRecordingKey",
        "editedRecordingFound",
    )),
    ("native VDR cut authority", plugin_vdr, (
        "SuiteBridgeRecordingCutState",
        "inspectLocked",
        "matchedRecording->IsInUse()",
        "nativeMarks.GetNumSequences()",
        "cCutter::EditedFileName",
        "RecordingsHandler.GetUsage",
        "SuiteBridgeRecordingCutStateCommand::Handle",
        "RecordingsHandler.Add(ruCut, finalRecording->FileName())",
    )),
    ("daemon exact result predicate", daemon_match, (
        "VdrRecordingNativeCutStateAvailability::Available",
        "state.editedRecordingFound",
        "state.editedDestinationExists",
        "state.editedRecordingKey == expectedEditedRecordingKey",
    )),
    ("daemon cut reconciliation", daemon_cut, (
        "recordingCutReconciliationCandidates",
        "resolver->resolve(candidate.recordingKey)",
        "daemonRecordingCutResultMatches",
        "verifyRecordingCutResult",
        "ensureRecordingCutReconciliationSchema",
    )),
    ("recording editing composition", daemon_editing, (
        "configureDaemonRecordingMarksRuntime",
        "configureDaemonRecordingCutRuntime",
        "resetDaemonRecordingCutRuntime",
        "resetDaemonRecordingMarksRuntime",
    )),
    ("daemon recording lifecycle", daemon_runtime, (
        "configureDaemonRecordingEditingRuntime",
        "resetDaemonRecordingEditingRuntime",
    )),
):
    for token in tokens:
        if token not in content:
            errors.append(f"{label} missing required token: {token}")

# The mutation boundary is intentionally singular and VDR-owned.
enqueue_token = "RecordingsHandler.Add(ruCut, finalRecording->FileName())"
if plugin_vdr.count(enqueue_token) != 1:
    errors.append("native cut authority must contain exactly one VDR ruCut enqueue")
if "cCutter::Start" in plugin_vdr or "cCuttingThread" in plugin_vdr:
    errors.append("SuiteBridge must not implement or invoke a custom cutter")

# Shared native inspection must collect every safety fact before the only enqueue.
enqueue = plugin_vdr.find(enqueue_token)
for token in (
    "matchedRecording->IsInUse()",
    "nativeMarks.GetNumSequences()",
    "cCutter::EditedFileName",
    "RecordingsHandler.GetUsage",
    "current.marksRevision != request.expectedMarksRevision",
    "finalState.marksRevision != request.expectedMarksRevision",
    "finalState.editedRecordingKey != current.editedRecordingKey",
):
    position = plugin_vdr.find(token)
    if position < 0 or enqueue < 0 or position >= enqueue:
        errors.append(f"native cut precondition must precede enqueue: {token}")

# Preview/read state is read-only: RCUT must be present, but the RCUT handler
# itself must not contain the native enqueue.
rcut_start = plugin_vdr.find("SuiteBridgeRecordingCutStateCommand::Handle")
start_cut = plugin_vdr.find("SuiteBridgeRecordingCutVdrMutationCallback::StartCut")
if rcut_start < 0 or start_cut < 0 or rcut_start >= start_cut:
    errors.append("RCUT read handler must be distinct from and precede NCUT mutation")
elif "RecordingsHandler.Add(" in plugin_vdr[rcut_start:start_cut]:
    errors.append("RCUT read handler must never enqueue a native cut")

# Durable local starting must be created and persisted before the receipt and
# the native dispatch in the actual recording-cut branch.
cut_branch = client.find("if (recordingCutCommand &&")
prepare = client.find("prepareFreshRecordingCutLocalStarting(", cut_branch)
receipt = client.find("sendReceipt(config, context, transport, state, reason)", prepare)
dispatch = client.find("executeFreshRecordingCutAndPersistOutcome(", receipt)
if min(cut_branch, prepare, receipt, dispatch) < 0 or not (
    cut_branch < prepare < receipt < dispatch
):
    errors.append("cut durable-start/receipt handoff must precede native dispatch")

# The executor gets exactly one native start call. Recovery and daemon result
# discovery are reconciliation-only and must never call the native start path.
if executor.count("transport.startCut(request)") != 1:
    errors.append("cut executor must make exactly one native start call")
if "startCut(" in local_state:
    errors.append("cut local-state recovery must never redispatch native cut")
if "startCut(" in daemon_cut or "NCUT" in daemon_cut:
    errors.append("daemon cut reconciliation must never redispatch native cut")

# RCUT is intentionally public and read-only; NCUT remains private.
help_start = plugin_svdrp.find("SVDRPHelpPages")
command_start = plugin_svdrp.find("SVDRPCommand", help_start)
help_section = plugin_svdrp[help_start:command_start] if help_start >= 0 else ""
if "RCUT <recording-key>" not in help_section:
    errors.append("read-only RCUT must be advertised in public SVDRP help")
if "NCUT" in help_section:
    errors.append("NCUT must not be advertised in public SVDRP help")

# No shell/process or browser/filesystem mutation path may enter the cut authority.
scoped = "\n".join((
    agent_main, client_header, client, domain, payload, assignment,
    local_state, executor, handler, transport, plugin_protocol, plugin_state,
    plugin_vdr, daemon_match, daemon_cut, daemon_editing,
))
for pattern in (
    r"\b(?:system|popen|fork|execl|execv|posix_spawn)\s*\(",
    r"\b(?:unlink|remove|rename)\s*\(",
):
    if re.search(pattern, scoped):
        errors.append(f"forbidden cut runtime boundary matched: {pattern}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("recording cut Slice-3 runtime wiring ok")
