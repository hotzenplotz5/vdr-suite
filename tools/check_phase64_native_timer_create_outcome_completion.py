#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent

files = {
    "dispatch header": ROOT / "core/timers/include/NativeTimerCreateDispatchService.h",
    "dispatch source": ROOT / "core/timers/src/NativeTimerCreateDispatchService.cpp",
    "completion header": ROOT / "core/timers/include/NativeTimerCreateOperationCompletionService.h",
    "completion source": ROOT / "core/timers/src/NativeTimerCreateOperationCompletionService.cpp",
    "test": ROOT / "core/timers/tests/test_native_timer_create_outcome_completion.cpp",
    "fragment": ROOT / "mk/phase64-native-timer-create-outcome-completion-tests.mk",
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
    "NativeTimerCreateExecutorOutcomeCategory",
    "NativeTimerCreateDispatchOutcomeResult",
    "NativeTimerCreateReadbackExpectation",
    "applyOutcome",
])
require("dispatch source", [
    "NativeTimerCreateExecutorOutcomeCategory::rejectedWithoutEffect",
    "NativeTimerCreateExecutorOutcomeCategory::acceptedUnverified",
    "NativeTimerCreateExecutorOutcomeCategory::outcomeUnknown",
    "MutationOperationState::failedVerified",
    "MutationOperationState::executedUnverified",
    "MutationOperationState::outcomeUnknown",
    "readbackNotBefore = outcome.dispatchStartedAt",
    "findPayloadByOperationId",
])
require("completion source", [
    "TimerAssignmentState::bound",
    "lastVerifiedOperationId == expectation.operationId",
    "nativeTimerObservationMatchesSpecification",
    "MutationOperationState::succeeded",
])
require("test", [
    "acceptedUnverified",
    "alreadyApplied",
    "TimerAssignmentFulfillmentStatus::bound",
    "alreadyCompleted",
])
require("fragment", [
    "test-phase64-native-timer-create-outcome-completion",
    "NativeTimerCreateOperationCompletionService.cpp",
    "test-fast: test-phase64-native-timer-create-outcome-completion",
    "test-architecture: test-phase64-native-timer-create-outcome-completion-architecture",
])
require("makefile", [
    "include mk/phase64-native-timer-create-outcome-completion-tests.mk",
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
    print("Phase-64 native Timer CREATE outcome/completion check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer CREATE outcome/completion check passed")
