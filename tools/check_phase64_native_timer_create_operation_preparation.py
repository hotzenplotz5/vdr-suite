#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "payload_h": "core/timers/include/NativeTimerCreateOperationPayload.h",
    "payload_cpp": "core/timers/src/NativeTimerCreateOperationPayload.cpp",
    "service_h": "core/timers/include/NativeTimerCreateOperationPreparationService.h",
    "service_cpp": "core/timers/src/NativeTimerCreateOperationPreparationService.cpp",
    "test": "core/timers/tests/test_native_timer_create_operation_preparation_service.cpp",
    "doc": "docs/development/phase-64-native-timer-create-operation-preparation.md",
    "mk": "mk/phase64-native-timer-create-operation-preparation-tests.mk",
    "makefile": "Makefile",
}
for relative in paths.values():
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing native Timer CREATE preparation file: {relative}")

payload_h = (ROOT / paths["payload_h"]).read_text(encoding="utf-8")
payload_cpp = (ROOT / paths["payload_cpp"]).read_text(encoding="utf-8")
service_h = (ROOT / paths["service_h"]).read_text(encoding="utf-8")
service_cpp = (ROOT / paths["service_cpp"]).read_text(encoding="utf-8")
test = (ROOT / paths["test"]).read_text(encoding="utf-8")
doc = (ROOT / paths["doc"]).read_text(encoding="utf-8")
mk = (ROOT / paths["mk"]).read_text(encoding="utf-8")
makefile = (ROOT / paths["makefile"]).read_text(encoding="utf-8")

for token in [
    "NativeTimerCreateOperationPayload",
    "expectedAssignmentRevision",
    "expectedIntentRevision",
    "assignmentEpoch",
    "nativeTimerBindingId",
    "backendGeneration",
    "NativeTimerSpecification",
    "serializeNativeTimerCreateOperationPayload",
    "parseNativeTimerCreateOperationPayload",
    "nativeTimerCreateOperationPayloadFingerprint",
]:
    if token not in payload_h:
        raise SystemExit(f"missing CREATE payload header marker: {token}")

for token in [
    'native-timer-create-operation-payload/1|',
    'native-timer-create-operation-payload-fingerprint/1|',
    "normalizedHhmm",
    "fields.reserve(18)",
    "serializeNativeTimerCreateOperationPayload(parsed) != serialized",
]:
    if token not in payload_cpp:
        raise SystemExit(f"missing CREATE payload codec marker: {token}")

for token in [
    "NativeTimerCreateOperationPreparationStatus",
    "expectedAssignmentEpoch",
    "expectedBackendId",
    "class NativeTimerCreateOperationPreparationService",
]:
    if token not in service_h:
        raise SystemExit(f"missing CREATE preparation header marker: {token}")

for token in [
    "intentRepository_.findById(assignment.timerIntentId)",
    "timerIntentAssignable(intent.state)",
    "assignment.assignmentRevision != request.expectedAssignmentRevision",
    "assignment.assignmentEpoch != request.expectedAssignmentEpoch",
    "assignment.state != TimerAssignmentState::provisioning",
    "!assignment.nativeTimerBindingId.empty()",
    "assignment.channelBinding.backendChannelId",
    'operation.resourceType = "TimerAssignment"',
    'operation.actionFamily = "timer.create"',
    "MutationOperationVerificationPolicy::readbackRequired",
    "operationRepository_.reserveWithPayload(operation, durablePayload)",
]:
    if token not in service_cpp:
        raise SystemExit(f"missing CREATE preparation source marker: {token}")

for forbidden in [
    "BackendAgent", "SuiteBridge", "SVDRP", "RestfulApi", "IHttpClient",
    "VdrTimerOperationRequest", "mutations=enabled",
]:
    if forbidden in payload_h + payload_cpp + service_h + service_cpp:
        raise SystemExit(f"premature CREATE preparation boundary crossing: {forbidden}")

for token in [
    "assertPayloadCodec",
    "assignmentStateConflict",
    "findPayloadByOperationId",
    "alreadyPrepared",
    "assignmentRevisionConflict",
    "intentRevisionConflict",
    "assignmentEpochConflict",
    "backendConflict",
    "generationConflict",
    "channelConflict",
    "operationConflict",
    "idempotencyConflict",
    "operationStateConflict",
    "test_native_timer_create_operation_preparation_service passed",
]:
    if token not in test:
        raise SystemExit(f"missing CREATE preparation regression marker: {token}")

for token in [
    "atomic operation + payload reservation",
    "provisioning",
    "TimerIntent",
    "assignment epoch",
    "restart",
    "no blind retry",
    "does not dispatch",
]:
    if token not in doc:
        raise SystemExit(f"missing CREATE preparation documentation marker: {token}")

for token in [
    "test-phase64-native-timer-create-operation-preparation-architecture",
    "test-phase64-native-timer-create-operation-preparation:",
    "test-fast: test-phase64-native-timer-create-operation-preparation",
    "test-architecture: test-phase64-native-timer-create-operation-preparation-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing CREATE preparation make marker: {token}")

if "include mk/phase64-native-timer-create-operation-preparation-tests.mk" not in makefile:
    raise SystemExit("CREATE preparation make fragment is not included")

# This slice must remain a Control-Plane contract only; runtime consumption is
# opened later together with durable dispatch/start fencing.
for scan_root in [
    ROOT / "apps", ROOT / "api", ROOT / "core" / "agent",
    ROOT / "core" / "daemon", ROOT / "core" / "http",
    ROOT / "core" / "runtime", ROOT / "core" / "vdr",
    ROOT / "vdr-plugin-suite-bridge",
]:
    if not scan_root.exists():
        continue
    for path in scan_root.rglob("*"):
        if not path.is_file() or path.suffix not in {
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc", ".mk"
        }:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "NativeTimerCreateOperationPreparationService" in text:
            raise SystemExit(
                "premature CREATE preparation runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 native Timer CREATE operation preparation check passed")
