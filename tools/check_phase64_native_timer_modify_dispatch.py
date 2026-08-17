#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent

files = {
    "dispatch header": ROOT / "core/timers/include/NativeTimerModifyDispatchService.h",
    "dispatch source": ROOT / "core/timers/src/NativeTimerModifyDispatchService.cpp",
    "completion header": ROOT / "core/timers/include/NativeTimerModifyOperationCompletionService.h",
    "completion source": ROOT / "core/timers/src/NativeTimerModifyOperationCompletionService.cpp",
    "readback header": ROOT / "core/timers/include/NativeTimerModifyReadbackVerificationService.h",
    "readback source": ROOT / "core/timers/src/NativeTimerModifyReadbackVerificationService.cpp",
    "test": ROOT / "core/timers/tests/test_native_timer_modify_dispatch.cpp",
    "fragment": ROOT / "mk/phase64-native-timer-modify-dispatch-tests.mk",
    "makefile": ROOT / "Makefile",
}

failures = []
for label, path in files.items():
    if not path.is_file():
        failures.append(f"missing {label}: {path.relative_to(ROOT)}")

def require(label, needles):
    path = files[label]
    if not path.is_file():
        return
    text = path.read_text(encoding="utf-8")
    for needle in needles:
        if needle not in text:
            failures.append(f"{label} missing required contract: {needle}")

require("dispatch header", [
    "NativeTimerModifyDispatchClaim",
    "NativeTimerModifyExecutorOutcomeCategory",
    "NativeTimerModifyReadbackExpectation",
    "applyOutcome",
])
require("dispatch source", [
    'payloadType != "native.timer.modify"',
    "findPayloadByOperationId",
    "expectedCurrentFingerprint",
    "MutationOperationState::dispatching",
    "MutationOperationState::executedUnverified",
    "MutationOperationState::outcomeUnknown",
    "MutationOperationState::failedVerified",
    "readbackNotBefore = outcome.dispatchStartedAt",
])
require("completion source", [
    "lastVerifiedOperationId == expectation.operationId",
    "nativeTimerObservationMatchesSpecification",
    "MutationOperationState::succeeded",
    "native-timer-modify-readback:",
])
require("readback header", [
    "NativeTimerReadbackOperationState operationState",
])
require("readback source", [
    "NativeTimerReadbackOperationState::executedUnverified",
    "NativeTimerReadbackOperationState::outcomeUnknown",
])
require("test", [
    "acceptedUnverified",
    "outcomeUnknown",
    "rejectedWithoutEffect",
    "alreadyClaimed",
    "alreadyCompleted",
])
require("fragment", [
    "test-phase64-native-timer-modify-dispatch",
    "NativeTimerModifyDispatchService.cpp",
    "NativeTimerModifyOperationCompletionService.cpp",
    "test-fast: test-phase64-native-timer-modify-dispatch",
    "test-architecture: test-phase64-native-timer-modify-dispatch-architecture",
])
require("makefile", [
    "include mk/phase64-native-timer-modify-dispatch-tests.mk",
])

for label in ("dispatch source", "completion source"):
    path = files[label]
    if not path.is_file():
        continue
    text = path.read_text(encoding="utf-8")
    for forbidden in ("BackendAgent", "SuiteBridge", "SVDRP", "system(", "popen("):
        if forbidden in text:
            failures.append(
                f"{label} contains forbidden transport coupling: {forbidden}"
            )

if failures:
    print("Phase-64 native Timer modify dispatch check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer modify dispatch check passed")
