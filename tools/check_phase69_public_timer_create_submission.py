#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "parser_h": ROOT / "api/rest/include/PublicTimerCreateRequestParser.h",
    "parser_cpp": ROOT / "api/rest/src/PublicTimerCreateRequestParser.cpp",
    "router": ROOT / "api/rest/include/ApiRouter.h",
    "server": ROOT / "core/http/src/TestHttpServer.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "daemon_h": ROOT / "core/daemon/include/DaemonRuntime.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "daemon_shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "submission_h": ROOT / "core/daemon/include/DaemonTimerCreateSubmissionService.h",
    "submission_cpp": ROOT / "core/daemon/src/DaemonTimerCreateSubmissionService.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "rest_sources": ROOT / "mk/rest-sources.mk",
    "api_test": ROOT / "api/rest/tests/test_public_timer_create_submission.cpp",
    "security_test": ROOT / "core/security/tests/test_public_timer_create_security.cpp",
    "daemon_test": ROOT / "core/daemon/tests/test_daemon_timer_create_submission_service.cpp",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C public Timer CREATE file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "public_cpp": [
        "publicEvaluateIfMatch(",
        "precondition_required",
        "revision_conflict",
        "Idempotency-Key",
        "idempotency_conflict",
        "validation_error",
        "acceptedOperationResponse(",
        'response.statusCode = 202;',
        'response.headers["Location"] = location;',
        "submitTimerCreate(submission)",
        "GET, POST",
        "public-api.timer-assignments-create",
    ],
    "parser_cpp": [
        'exactKeys(root.objectValue, {"nativeTimer"})',
        '"title", "directory", "day", "weekdays"',
        '"startTime", "endTime", "priority", "lifetime"',
        '"enabled", "vps"',
        "invalidRequest",
        "validationError",
    ],
    "router": [
        "idempotencyKey",
        "ifMatch",
        "authorizedBackendRef",
        "PublicApiRuntime::instance().tryHandlePost(",
    ],
    "server": [
        '"Idempotency-Key"',
        '"If-Match"',
        "gate.authorizationDecision.backendId",
    ],
    "security": [
        "isPublicTimerAssignmentCreate",
        'timerCreateRequest.permission = "timers.create";',
        'timerCreateRequest.action = "timers.create";',
        "timerCreateRequest.backendId =",
        "gate.protectedMutation = true;",
    ],
    "daemon_h": [
        "DaemonTimerCreateSubmissionService",
        "daemonTimerCreateSubmissionService_",
    ],
    "daemon_init": [
        "std::make_unique<DaemonTimerCreateSubmissionService>",
        "registerTimerCreateSubmission(",
        "daemonTimerCreateSubmissionService_->submit(",
    ],
    "daemon_shutdown": [
        "resetTimerCreateSubmission();",
        "daemonTimerCreateSubmissionService_.reset();",
    ],
    "submission_cpp": [
        "findByIdempotencyScope(",
        '"TimerAssignment"',
        '"timer.create"',
        'backendAgentGenerateOpaqueId("op_", 12)',
        'backendAgentGenerateOpaqueId("ntb_", 12)',
        "preparationService_.prepare(preparation)",
        "reservationService_.reserve(",
        "dispatchService_.claimAfterReservation(",
        "activationService_.activateDispatching(",
        "findPayloadByOperationId(",
        'storedPayload.payload.payloadType != "native.timer.create"',
        "operation.state == MutationOperationState::dispatching",
        "operation.state != MutationOperationState::accepted",
        "request.requestedSpecification.channelId.empty()",
        "assignment.channelBinding.backendChannelId",
        "ActorType::System",
        "sha256:",
    ],
    "api_test": [
        "statusCode == 428",
        "statusCode == 412",
        "statusCode == 422",
        "statusCode == 202",
        '"Location"',
        "idempotency_conflict",
    ],
    "security_test": [
        "decision.protectedMutation",
        'decision.authorizationDecision.permission ==',
        '"timers.create"',
    ],
    "daemon_test": [
        "first.operation.state ==",
        "MutationOperationState::dispatching",
        "replay.operation.operationId ==",
        "first.operation.operationId",
        "replayCommand->commandId == firstCommandId",
        "DaemonTimerCreateSubmissionStatus::idempotencyConflict",
    ],
}

for label, markers in required.items():
    for marker in markers:
        if marker not in contents[label]:
            raise SystemExit(
                f"missing public Timer CREATE marker in {label}: {marker}")

# The public request owns desired native presentation/timing only. Backend and
# concurrency identities remain server-side and may not become public fields.
public_surface = contents["parser_h"] + contents["parser_cpp"]
for forbidden in [
    '"channelId"',
    '"backendId"',
    '"backendGeneration"',
    '"assignmentRevision"',
    '"intentRevision"',
    '"assignmentEpoch"',
    '"nativeTimerBindingId"',
    '"operationId"',
]:
    if forbidden in public_surface:
        raise SystemExit(
            f"internal Timer CREATE fence leaked into public request schema: {forbidden}")

# Backend scope must come from the authorization decision, never from reparsing
# or from the public JSON body in PublicApiRuntime.
if "queryStringValue" in contents["public_cpp"]:
    raise SystemExit(
        "PublicApiRuntime must not reparse backend scope for Timer CREATE")
if 'jsonStringValue' in contents["public_cpp"]:
    raise SystemExit(
        "PublicApiRuntime must not derive Timer CREATE backend from request JSON")

# One integration owner only.
if contents["daemon_h"].count(
        "std::unique_ptr<DaemonTimerCreateSubmissionService>") != 1:
    raise SystemExit(
        "DaemonRuntime must own exactly one Timer CREATE submission service")
if contents["daemon_sources"].count(
        "core/daemon/src/DaemonTimerCreateSubmissionService.cpp") != 1:
    raise SystemExit(
        "Timer CREATE submission service must be linked exactly once")
if contents["rest_sources"].count(
        "api/rest/src/PublicTimerCreateRequestParser.cpp") != 1:
    raise SystemExit(
        "public Timer CREATE parser must be linked exactly once")

# The public integration must reuse the accepted Phase-64 durable chain and must
# not bypass it via a legacy RESTfulAPI/VDR/SVDRP mutation path.
for forbidden in [
    "RestfulApiVdrTimerAction",
    "RestfulApiTimer",
    "VdrTimerAction",
    "SuiteBridgeSvdrp",
    "SVDRP",
    "BasicHttpClient",
    "Legacy",
]:
    if forbidden in contents["submission_cpp"]:
        raise SystemExit(
            f"public Timer CREATE bypasses accepted durable chain: {forbidden}")

# IDs are generated only after the idempotency scope lookup; this prevents a
# retry from manufacturing a replacement operation/command identity.
lookup_pos = contents["submission_cpp"].find("findByIdempotencyScope(")
operation_id_pos = contents["submission_cpp"].find(
    'backendAgentGenerateOpaqueId("op_", 12)')
binding_id_pos = contents["submission_cpp"].find(
    'backendAgentGenerateOpaqueId("ntb_", 12)')
if not (
    0 <= lookup_pos < operation_id_pos and
    0 <= lookup_pos < binding_id_pos
):
    raise SystemExit(
        "Timer CREATE identities must be generated only after idempotency lookup")

# Durable accepted replay must recover immutable payload rather than calling
# prepare() with a newly generated timestamp/deadline.
payload_lookup_pos = contents["submission_cpp"].find(
    "findPayloadByOperationId(")
prepare_pos = contents["submission_cpp"].find(
    "preparationService_.prepare(preparation)")
if not (0 <= payload_lookup_pos < prepare_pos):
    raise SystemExit(
        "accepted Timer CREATE replay must recover durable payload before new preparation")

# The orchestration order for a fresh operation is fixed.
reservation_pos = contents["submission_cpp"].find(
    "reservationService_.reserve(")
claim_pos = contents["submission_cpp"].find(
    "dispatchService_.claimAfterReservation(")
activation_pos = contents["submission_cpp"].rfind(
    "activationService_.activateDispatching(")
if not (0 <= prepare_pos < reservation_pos < claim_pos < activation_pos):
    raise SystemExit(
        "Timer CREATE orchestration must remain prepare -> reserve -> claim -> activate")

# Public operation ownership remains the durable MutationOperation. The API must
# never report a synthesized success resource.
for forbidden in [
    "succeeded_without_operation",
    "syntheticOperation",
    "fallbackOperation",
]:
    if forbidden in contents["public_cpp"] + contents["submission_cpp"]:
        raise SystemExit(
            f"unsafe synthetic Timer CREATE outcome marker: {forbidden}")

print("Phase-69.C public Timer CREATE submission check passed")
print("Boundary: authorized If-Match + idempotent durable prepare/reserve/claim/activate; no legacy fallback")
