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
command_admin = text("apps/tools/backend_agent_command_admin.cpp")
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
plugin_capabilities = text("vdr-plugin-suite-bridge/suitebridge_capabilities.cpp")
daemon_match = text("core/daemon/src/DaemonRecordingCutReconciliation.cpp")
daemon_cut = text("core/daemon/src/DaemonRuntimeRecordingCut.cpp")
daemon_editing = text("core/daemon/src/DaemonRuntimeRecordingEditing.cpp")
daemon_runtime = text("core/daemon/src/DaemonRuntime.cpp")
api_router = text("api/rest/include/ApiRouter.h")
api_runtime = text("api/rest/src/RecordingCutApiRuntime.cpp")
security_gate = text("core/security/include/SecurityHttpGate.h")
authorization = text("core/security/include/AuthorizationService.h")
acceptance_preflight = text("tools/run_recording_cut_acceptance_preflight.sh")
cut_make = text("mk/recording-native-cut-guard.mk")

for label, content, tokens in (
    ("shipped Agent", agent_main, (
        "SuiteBridgeRecordingCutTransport",
        "kBackendAgentRecordingCutCommandType",
        "setBackendAgentRecordingCutTransport(recordingCutTransport.get())",
    )),
    ("cut ownership admin", command_admin, (
        '"BackendAgentRecordingCut.h"',
        '"--recording-cut-provider-ownership-status"',
        '"--set-recording-cut-owner"',
        '"--clear-recording-cut-owner"',
        "kBackendAgentRecordingCutAuthorityDomain",
        "kBackendAgentRecordingCutProviderId",
        "kBackendAgentRecordingCutProviderKind",
        "kBackendAgentRecordingCutCapability",
        "setLocalProviderOwnership",
        "clearLocalProviderOwnership",
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
    ("SuiteBridge cut-state discovery", plugin_capabilities, (
        '"recording-cut-state"',
        "SuiteBridgeCapabilityState::Available",
    )),
    ("native VDR cut authority", plugin_vdr, (
        "SuiteBridgeRecordingCutState",
        "inspectLocked",
        "const cRecordings *recordings",
        "recordings->First()",
        "recordings->Next(candidate)",
        "inspectLocked(Recordings, recordingKey, recording)",
        "inspectLocked(Recordings, request.recordingKey, recording)",
        "inspectLocked(Recordings, request.recordingKey, finalRecording)",
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
        'capabilityAvailable(\n                "recording-cut-state")',
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
    ("recording cut API router", api_router, (
        "RecordingCutApiRuntime::instance().tryHandleGet",
        "RecordingCutApiRuntime::instance().tryHandlePost",
    )),
    ("recording cut API runtime", api_runtime, (
        '"/api/vdr/recordings/cut"',
        "expectedMarksRevision",
        "VdrRecordingNativeIdentity::keyForNativeId",
        "state.ready",
        "replayOnly",
        "readback_required",
    )),
    ("recording cut HTTP security", security_gate, (
        "isRecordingCutAction",
        'path == "/api/vdr/recordings/cut"',
        'requestToAuthorize.permission = "recordings.cut"',
        'requestToAuthorize.action = "recordings.cut"',
    )),
    ("recording cut authorization", authorization, (
        'permission == "recordings.cut"',
        "protectedMutationPermission",
        "isReadOnlyMutation",
    )),
    ("recording cut acceptance preflight", acceptance_preflight, (
        'ADMIN_CANDIDATE=".build/vdr-suite-backend-agent-command-admin"',
        'ADMIN_INSTALLED="/usr/sbin/vdr-suite-backend-agent-command-admin"',
        'cmp -s "$ADMIN_CANDIDATE" "$ADMIN_INSTALLED"',
        "--recording-cut-provider-ownership-status",
        "recording-cut-provider-ownership.json",
        'ownership.get("providerId") == "suitebridge:recording-cut"',
        'ownership.get("allowedCapabilities") == ["vdr.recording.cut"]',
        '"local_provider_ownership_active"',
    )),
):
    for token in tokens:
        if token not in content:
            errors.append(f"{label} missing required token: {token}")

# The real-system preflight may observe cut ownership, but must never mutate it.
for forbidden in (
    "--set-recording-cut-owner",
    "--clear-recording-cut-owner",
):
    if forbidden in acceptance_preflight:
        errors.append(f"recording cut acceptance preflight contains forbidden ownership mutation: {forbidden}")

# The VDR recordings view supplied by LOCK_RECORDINGS_READ is local to the
# lock-owning scope. Shared inspection must receive that already-locked view
# explicitly instead of referring to the macro-local Recordings identifier.
if "Recordings->First()" in plugin_vdr or "Recordings->Next(" in plugin_vdr:
    errors.append("native cut inspection must use the explicitly passed locked recordings view")

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

# The HTTP owner may dispatch only through the typed Control Plane assignment;
# it must never know or emit NCUT, VDR paths, or native cutter calls.
for forbidden in (
    "NCUT",
    "RecordingsHandler.Add",
    "cCutter::",
    "FileName()",
):
    if forbidden in api_runtime or forbidden in api_router:
        errors.append(f"HTTP cut boundary contains forbidden native authority: {forbidden}")

# Cut POST must be both routed and mapped to an explicit protected permission.
if api_router.count("RecordingCutApiRuntime::instance().tryHandlePost") != 1:
    errors.append("recording cut POST must have exactly one API router owner")
if security_gate.count('path == "/api/vdr/recordings/cut"') != 1:
    errors.append("recording cut POST must have exactly one SecurityHttpGate route mapping")

# RCUT is intentionally public and read-only; NCUT remains private.
help_start = plugin_svdrp.find("SVDRPHelpPages")
command_start = plugin_svdrp.find("SVDRPCommand", help_start)
help_section = plugin_svdrp[help_start:command_start] if help_start >= 0 else ""
if "RCUT <recording-key>" not in help_section:
    errors.append("read-only RCUT must be advertised in public SVDRP help")
if "NCUT" in help_section:
    errors.append("NCUT must not be advertised in public SVDRP help")

# The complete automated Slice-3 edge must remain mandatory in test-fast.
for target in (
    "test-recording-cut-api-runtime",
    "test-recording-cut-security",
    "test-backend-agent-recording-cut",
    "test-backend-agent-recording-cut-local-state",
    "test-backend-agent-recording-cut-executor",
    "test-backend-agent-recording-cut-reconciliation",
    "test-suite-bridge-svdrp-recording-cut-transport",
    "test-suite-bridge-svdrp-recording-cut-state-transport",
    "test-suite-bridge-recording-cut-state-resolver",
    "test-suitebridge-recording-cut-protocol",
    "test-daemon-recording-cut-reconciliation",
):
    if target not in cut_make:
        errors.append(f"Slice-3 test-fast contract missing target: {target}")

# No shell/process or browser/filesystem mutation path may enter the cut authority.
scoped = "\n".join((
    agent_main, client_header, client, domain, payload, assignment,
    local_state, executor, handler, transport, plugin_protocol, plugin_state,
    plugin_vdr, daemon_match, daemon_cut, daemon_editing, api_router,
    api_runtime, security_gate,
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
