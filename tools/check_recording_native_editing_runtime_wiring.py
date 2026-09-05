#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

context_path = ROOT / "core/daemon/include/BackendRuntimeContext.h"
make_path = ROOT / "mk/recording-native-editing-tests.mk"
daemon_sources_path = ROOT / "mk/daemon-sources.mk"
api_header_path = ROOT / "api/rest/include/RecordingMarksApiRuntime.h"
api_source_path = ROOT / "api/rest/src/RecordingMarksApiRuntime.cpp"
router_path = ROOT / "api/rest/include/ApiRouter.h"
daemon_path = ROOT / "core/daemon/src/DaemonRuntime.cpp"
daemon_marks_path = ROOT / "core/daemon/src/DaemonRuntimeRecordingMarks.cpp"
security_gate_path = ROOT / "core/security/include/SecurityHttpGate.h"
authorization_path = ROOT / "core/security/include/AuthorizationService.h"
agent_main_path = ROOT / "apps/agent/main.cpp"
agent_client_path = ROOT / "core/agent/src/BackendAgentCommandClient.cpp"
admin_path = ROOT / "apps/tools/backend_agent_command_admin.cpp"

paths = (
    context_path,
    make_path,
    daemon_sources_path,
    api_header_path,
    api_source_path,
    router_path,
    daemon_path,
    daemon_marks_path,
    security_gate_path,
    authorization_path,
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
daemon_sources = daemon_sources_path.read_text(encoding="utf-8")
api_header = api_header_path.read_text(encoding="utf-8")
api_source = api_source_path.read_text(encoding="utf-8")
router = router_path.read_text(encoding="utf-8")
daemon = daemon_path.read_text(encoding="utf-8")
daemon_marks = daemon_marks_path.read_text(encoding="utf-8")
security_gate = security_gate_path.read_text(encoding="utf-8")
authorization = authorization_path.read_text(encoding="utf-8")
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
    "$(RECORDING_NATIVE_EDITING_REST_SRC)",
    "DAEMON_SRC += $(VDR_RECORDING_NATIVE_MARKS_SRC)",
    "DAEMON_SRC += $(RECORDING_NATIVE_EDITING_REST_SRC)",
    "REST_ROUTER_SRC += $(RECORDING_NATIVE_EDITING_ROUTER_SRC)",
    "test-recording-marks-api-runtime:",
    "test-backend-agent-recording-marks-modify-assignment:",
    "test-suite-bridge-svdrp-recording-marks-modify-transport:",
    "check-suitebridge-recording-marks-vdr-mutation:",
    "check-recording-native-editing-runtime-wiring:",
    "python3 tools/check_recording_native_editing_runtime_wiring.py",
)
for fragment in required_make:
    if fragment not in makefile:
        errors.append(f"missing native marks build wiring: {fragment}")

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
    "parseMutationRequest(",
    "expectedMarksRevision",
    "RecordingMarksMutationKind::Add",
    "RecordingMarksMutationKind::Delete",
    "RecordingMarksMutationKind::Move",
    "RecordingMarksMutationKind::Reset",
    "RecordingMarksMutationKind::Replace",
    "backendWritePolicy_(request.backendId)",
    "nativeMarks.inUseFlags != 0",
    "nativeMarks.marksRevision != request.expectedMarksRevision",
    "mutationDispatcher_(mutation)",
    'response.statusCode = 202',
    '"readback_required"',
)
for fragment in required_api_mutation:
    if fragment not in api_source:
        errors.append(f"missing Slice-2 marks mutation API contract: {fragment}")

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
    '#include "RecordingMarksApiRuntime.h"',
    "RecordingMarksApiRuntime::instance().configure(",
    "findAllForBackend(",
    'capabilityAvailable(\n                        "recording-marks")',
    "ensureRecordingMarksResolver()",
    "canWriteToBackend(*registry, backendId)",
    "findAgentForBackend(request.backendId)",
    "assignmentRequest.backendGeneration = agent->backendGeneration",
    "BackendAgentRecordingMarksModifyAssignmentService",
    "assignmentService.assign(",
    "RecordingMarksApiRuntime::instance().reset();",
)
for fragment in required_daemon_marks:
    if fragment not in daemon_marks:
        errors.append(f"missing Recording marks daemon runtime wiring: {fragment}")

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
    if forbidden in context or forbidden in api_source or forbidden in api_header or forbidden in daemon_marks:
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
    if forbidden_native_write in api_source or forbidden_native_write in daemon_marks:
        errors.append(
            f"Control Plane attempted direct native marks write: {forbidden_native_write}"
        )

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("recording native editing Slice-2 runtime wiring ok")
