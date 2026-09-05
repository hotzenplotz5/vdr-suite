#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

context_path = ROOT / "core/daemon/include/BackendRuntimeContext.h"
make_path = ROOT / "mk/recording-native-editing-tests.mk"
agent_sources_path = ROOT / "mk/agent-sources.mk"
security_make_path = ROOT / "mk/security-sources.mk"
daemon_sources_path = ROOT / "mk/daemon-sources.mk"
api_header_path = ROOT / "api/rest/include/RecordingMarksApiRuntime.h"
api_source_path = ROOT / "api/rest/src/RecordingMarksApiRuntime.cpp"
api_test_path = ROOT / "api/rest/tests/test_recording_marks_api_runtime.cpp"
router_path = ROOT / "api/rest/include/ApiRouter.h"
daemon_path = ROOT / "core/daemon/src/DaemonRuntime.cpp"
daemon_marks_path = ROOT / "core/daemon/src/DaemonRuntimeRecordingMarks.cpp"
reconciliation_path = ROOT / "core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp"
reconciliation_test_path = ROOT / "core/agent/tests/test_backend_agent_recording_marks_modify_reconciliation.cpp"
security_gate_path = ROOT / "core/security/include/SecurityHttpGate.h"
authorization_path = ROOT / "core/security/include/AuthorizationService.h"
security_test_path = ROOT / "core/security/tests/test_recording_marks_security.cpp"
agent_main_path = ROOT / "apps/agent/main.cpp"
agent_client_path = ROOT / "core/agent/src/BackendAgentCommandClient.cpp"
admin_path = ROOT / "apps/tools/backend_agent_command_admin.cpp"

paths = (
    context_path,
    make_path,
    agent_sources_path,
    security_make_path,
    daemon_sources_path,
    api_header_path,
    api_source_path,
    api_test_path,
    router_path,
    daemon_path,
    daemon_marks_path,
    reconciliation_path,
    reconciliation_test_path,
    security_gate_path,
    authorization_path,
    security_test_path,
    agent_main_path,
    agent_client_path,
    admin_path,
)

errors = []
for path in paths:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

context = context_path.read_text(encoding="utf-8")
makefile = make_path.read_text(encoding="utf-8")
agent_sources = agent_sources_path.read_text(encoding="utf-8")
security_make = security_make_path.read_text(encoding="utf-8")
daemon_sources = daemon_sources_path.read_text(encoding="utf-8")
api_header = api_header_path.read_text(encoding="utf-8")
api_source = api_source_path.read_text(encoding="utf-8")
api_test = api_test_path.read_text(encoding="utf-8")
router = router_path.read_text(encoding="utf-8")
daemon = daemon_path.read_text(encoding="utf-8")
daemon_marks = daemon_marks_path.read_text(encoding="utf-8")
reconciliation = reconciliation_path.read_text(encoding="utf-8")
reconciliation_test = reconciliation_test_path.read_text(encoding="utf-8")
security_gate = security_gate_path.read_text(encoding="utf-8")
authorization = authorization_path.read_text(encoding="utf-8")
security_test = security_test_path.read_text(encoding="utf-8")
agent_main = agent_main_path.read_text(encoding="utf-8")
agent_client = agent_client_path.read_text(encoding="utf-8")
admin = admin_path.read_text(encoding="utf-8")

required_context = (
    '#include "SuiteBridgeRecordingMarksResolver.h"',
    "SuiteBridgeRecordingMarksResolver> recordingMarksResolver",
    "SuiteBridgeRecordingMarksResolver* ensureRecordingMarksResolver()",
    "if (!suiteBridgeTransport)",
    "if (!recordingMarksResolver)",
    "std::make_unique<SuiteBridgeRecordingMarksResolver>",
    "*suiteBridgeTransport",
    "return recordingMarksResolver.get();",
)
for fragment in required_context:
    if fragment not in context:
        errors.append(f"missing BackendRuntimeContext marks wiring: {fragment}")

required_make = (
    "VDR_RECORDING_NATIVE_MARKS_SRC :=",
    "core/vdr/src/SuiteBridgeRecordingMarksResolver.cpp",
    "RECORDING_NATIVE_EDITING_REST_SRC :=",
    "api/rest/src/RecordingMarksApiRuntime.cpp",
    "RECORDING_NATIVE_EDITING_ROUTER_SRC :=",
    "core/vdr/src/VdrRecordingNativeIdentity.cpp",
    "RECORDING_NATIVE_EDITING_AGENT_MARKS_MODIFY_TRANSPORT_SRC :=",
    "core/agent/src/SuiteBridgeSvdrpRecordingMarksModifyTransport.cpp",
    "DAEMON_SRC += $(VDR_RECORDING_NATIVE_MARKS_SRC)",
    "DAEMON_SRC += $(RECORDING_NATIVE_EDITING_REST_SRC)",
    "REST_ROUTER_SRC += $(RECORDING_NATIVE_EDITING_ROUTER_SRC)",
    "test-recording-marks-api-runtime:",
    "test-backend-agent-recording-marks-modify-assignment:",
    "test-backend-agent-recording-marks-modify-reconciliation:",
    "core/agent/tests/test_backend_agent_recording_marks_modify_reconciliation.cpp",
    "test-suite-bridge-svdrp-recording-marks-modify-transport:",
    "check-suitebridge-recording-marks-vdr-mutation:",
    "check-recording-native-editing-runtime-wiring:",
    "python3 tools/check_recording_native_editing_runtime_wiring.py",
)
for fragment in required_make:
    if fragment not in makefile:
        errors.append(f"missing native marks build wiring: {fragment}")

if "core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp" not in agent_sources:
    errors.append("missing recording marks reconciliation production source wiring")

for fragment in (
    "test-security-recording-marks:",
    "core/security/tests/test_recording_marks_security.cpp",
    "test-security-recording-marks \\",
):
    if fragment not in security_make:
        errors.append(f"missing Recording marks security test wiring: {fragment}")

if "core/daemon/src/DaemonRuntimeRecordingMarks.cpp" not in daemon_sources:
    errors.append(
        "missing Recording marks daemon source wiring: "
        "core/daemon/src/DaemonRuntimeRecordingMarks.cpp"
    )

required_api_read = (
    '"/api/vdr/recordings/marks"',
    "recording.id != request.recordingId",
    "recording.backendId != request.backendId",
    "VdrRecordingNativeIdentity::keyForNativeId(",
    "selected->backendNativeId",
    "access.resolver->resolve(recordingKey)",
    "recording_marks_capability_unavailable",
    "recording_native_state_stale",
    "recording_marks_invalid_native_payload",
    'response.headers["Cache-Control"] = "no-store"',
)
for fragment in required_api_read:
    if fragment not in api_source:
        errors.append(f"missing Recording marks read API contract: {fragment}")

required_api_mutation = (
    "RecordingMarksApiRuntime::tryHandlePost(",
    "parseMutationBody(body, request)",
    "expectedMarksRevision",
    "RecordingMarksMutationKind::Add",
    "RecordingMarksMutationKind::Delete",
    "RecordingMarksMutationKind::Move",
    "RecordingMarksMutationKind::Reset",
    "RecordingMarksMutationKind::Replace",
    "backendWritePolicy(request.request.backendId)",
    "nativeMarks.inUseFlags != 0",
    "nativeMarks.marksRevision != request.mutation.expectedMarksRevision",
    "replayRequest.replayOnly = true",
    "mutationDispatcher(replayRequest)",
    "ReplayNotFoundReason",
    "mutationDispatcher(request.mutation)",
    "validAcceptedDispatch(",
    "response.statusCode = dispatch.verified ? 200 : 202",
    "readback_required",
)
for fragment in required_api_mutation:
    if fragment not in api_source:
        errors.append(f"missing Slice-2 marks mutation API contract: {fragment}")

for fragment in (
    "int replayProbeCalls = 0;",
    "bool replayExists = false;",
    "request.replayOnly",
    '"recording_marks_modify_assignment_not_found"',
    'resolver.next.marksRevision =',
    '"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"',
    'response.body.find("\\\"replayed\\\":true")',
):
    if fragment not in api_test:
        errors.append(f"missing public marks replay regression: {fragment}")

required_router = (
    '#include "RecordingMarksApiRuntime.h"',
    "RecordingMarksApiRuntime::instance().tryHandleGet(",
    "RecordingMarksApiRuntime::instance().tryHandlePost(",
)
for fragment in required_router:
    if fragment not in router:
        errors.append(f"missing Recording marks router wiring: {fragment}")

required_daemon_lifecycle = (
    '#include "DaemonRuntimeRecordingMarks.h"',
    "configureDaemonRecordingMarksRuntime(",
    "*vdrRecordingCacheRepository_",
    "backendRuntimeContexts_",
    "*backendRegistryService_",
    "*backendAccessPolicy_",
    "*backendAgentRepository_",
    "*backendAgentCommandRepository_",
    "resetDaemonRecordingMarksRuntime();",
)
for fragment in required_daemon_lifecycle:
    if fragment not in daemon:
        errors.append(f"missing Recording marks daemon lifecycle wiring: {fragment}")

required_daemon_marks = (
    '#include "BackendAccessPolicy.h"',
    '#include "BackendAgentRecordingMarksModifyAssignment.h"',
    '#include "BackendAgentRecordingMarksModifyPayload.h"',
    '#include "RecordingMarksApiRuntime.h"',
    "RecordingMarksApiRuntime::instance().configure(",
    "findAllForBackend(",
    'capabilityAvailable(\n                        "recording-marks")',
    "ensureRecordingMarksResolver()",
    "canWriteToBackend(*registry, backendId)",
    "findAgentForBackend(request.backendId)",
    "assignmentRequest.backendGeneration = agent->backendGeneration",
    "findAssignmentForOperation(",
    "else if (request.replayOnly)",
    '"recording_marks_modify_assignment_not_found"',
    "recordingMarksModifyVerificationForOperation(",
    "dispatch.verified = true",
    '"recording_marks_modify_verified_replayed"',
    "backendAgentRecordingMarksModifyParsePayload(",
    "existingPayload.controlPlaneClaimedAt",
    "BackendAgentRecordingMarksModifyAssignmentService",
    "assignmentService.assign(",
    "request.replayOnly && assigned.accepted && !assigned.replayed",
    "ensureRecordingMarksModifyReconciliationSchema()",
    "recordingMarksModifyReconciliationCandidates()",
    "resolver->resolve(candidate.recordingKey)",
    "nativeMarks.recordingKey != candidate.recordingKey",
    "nativeMarks.marksRevision == candidate.expectedMarksRevision",
    "verifyRecordingMarksModifyReadback(",
    "startRecordingMarksReconciliation(",
    "stopRecordingMarksReconciliation();",
    "RecordingMarksApiRuntime::instance().reset();",
)
for fragment in required_daemon_marks:
    if fragment not in daemon_marks:
        errors.append(f"missing Recording marks daemon runtime wiring: {fragment}")

required_reconciliation = (
    "backend_agent_recording_marks_modify_readbacks",
    "recordingMarksModifyReconciliationCandidates()",
    'c.state=\'waiting_reconciliation\'',
    'x.dispatch_state=\'accepted_by_executor\'',
    'x.verification_state=\'outcome_unknown\'',
    'x.result_category=\'outcome_unknown\'',
    'x.retry_classification=\'reconcile_only\'',
    "localProviderSelectionCurrent(commandId, providerReason)",
    "expectedCanonicalRevision(",
    "canonical_marks_revision",
    "canonicalMarksRevision == expectedMarksRevision",
    "canonicalMarksRevision != postRevision",
    "observedAt < executorCompletedAt",
    "state='completed'",
)
for fragment in required_reconciliation:
    if fragment not in reconciliation:
        errors.append(f"missing authoritative marks reconciliation contract: {fragment}")

for fragment in (
    '"accepted_by_executor", clock + 3',
    '"starting", clock + 3',
    "recording_marks_modify_readback_state_mismatch",
    "recordingMarksModifyReconciliationCandidates().empty()",
    "recordingMarksModifyVerificationForOperation(",
    "recording_marks_modify_readback_verified",
    "recording_marks_modify_readback_replayed",
    "op_marks_stale_provider",
):
    if fragment not in reconciliation_test:
        errors.append(f"missing marks reconciliation regression: {fragment}")

required_security = (
    'path == "/api/vdr/recordings/marks"',
    'requestToAuthorize.permission = "recordings.marks.modify"',
    'requestToAuthorize.action = "recordings.marks.modify"',
)
for fragment in required_security:
    if fragment not in security_gate:
        errors.append(f"missing protected marks HTTP policy: {fragment}")
if 'permission == "recordings.marks.modify"' not in authorization:
    errors.append("recordings.marks.modify missing from protected mutation permissions")
for fragment in (
    "csrf_validation_failed",
    "recordings.marks.modify",
    "backend_scope_denied",
    "role_read_only",
):
    if fragment not in security_test:
        errors.append(f"missing marks HTTP security regression: {fragment}")

required_agent = (
    "SuiteBridgeRecordingMarksModifyTransport",
    "kBackendAgentRecordingMarksModifyCommandType",
    "setBackendAgentRecordingMarksModifyTransport(",
)
for fragment in required_agent:
    if fragment not in agent_main:
        errors.append(f"missing shipped Agent marks activation: {fragment}")

required_agent_client = (
    "RecordingMarksModifyTransport",
    "reconcileRecordingMarksModifyLocalState(",
    "prepareFreshRecordingMarksModifyLocalStarting(",
    "executeFreshRecordingMarksModifyAndPersistOutcome(",
    "kBackendAgentRecordingMarksModifyCapability",
)
for fragment in required_agent_client:
    if fragment not in agent_client:
        errors.append(f"missing Agent marks command handoff: {fragment}")

required_ownership_admin = (
    '"--recording-marks-provider-ownership-status"',
    '"--set-recording-marks-owner"',
    '"--clear-recording-marks-owner"',
    "kBackendAgentRecordingMarksModifyAuthorityDomain",
    "kBackendAgentRecordingMarksModifyProviderId",
    "kBackendAgentRecordingMarksModifyProviderKind",
    "kBackendAgentRecordingMarksModifyCapability",
)
for fragment in required_ownership_admin:
    if fragment not in admin:
        errors.append(f"missing explicit marks provider ownership admin: {fragment}")

for forbidden in (
    "RecordingActionType::Cut",
    "RecordingsHandler.Add",
    "cCutter",
):
    if (forbidden in context or forbidden in api_source or forbidden in api_header or
            forbidden in daemon_marks or forbidden in reconciliation):
        errors.append(f"Slice 3 mutation leaked into Slice 2: {forbidden}")

for forbidden_query in (
    'key == "backendNativeId"',
    'key == "path"',
    'key == "recordingPath"',
):
    if forbidden_query in api_source:
        errors.append(f"client native-path authority exposed: {forbidden_query}")

for forbidden_native_write in (
    "fopen(",
    "unlink(",
    "remove(",
    "system(",
    "popen(",
):
    if (forbidden_native_write in api_source or
            forbidden_native_write in daemon_marks or
            forbidden_native_write in reconciliation):
        errors.append(
            f"Control Plane attempted direct native marks write: {forbidden_native_write}"
        )

for forbidden_reconciliation in (
    "requestReplay(",
    "BackendAgentRecordingMarksModifyAssignmentService",
):
    if forbidden_reconciliation in reconciliation:
        errors.append(
            "authoritative marks reconciliation attempted mutation/retry path: "
            f"{forbidden_reconciliation}"
        )

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("recording native editing Slice-2 runtime wiring ok")
