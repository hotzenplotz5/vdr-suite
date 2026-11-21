#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing native Timer modify file: {path}")
    return target.read_text(encoding="utf-8")

def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")

makefile = read("Makefile")
payload = read("core/timers/src/NativeTimerModifyOperationPayload.cpp")
preparation = read(
    "core/timers/src/NativeTimerModifyOperationPreparationService.cpp"
)
verification = read(
    "core/timers/src/NativeTimerModifyReadbackVerificationService.cpp"
)
test = read("core/timers/tests/test_native_timer_modify_operation.cpp")
doc = read("docs/development/phase-64-native-timer-modify-operation.md")

include = "include mk/phase64-native-timer-modify-operation-tests.mk"
require(makefile, include, "modify operation Make include")
if makefile.count(include) != 1:
    raise SystemExit("modify operation Make include must occur exactly once")

for needle in (
    "native-timer-modify-operation-payload/1|",
    "expectedCurrentFingerprint",
    "expectedBindingRevision",
    "NativeTimerModifyKind::update",
    "NativeTimerModifyKind::toggle",
):
    require(payload, needle, "durable UPDATE/TOGGLE payload")

for needle in (
    "TimerAssignmentState::bound",
    "NativeTimerBindingOwnership::managed",
    "NativeTimerBindingOwnership::adopted",
    "binding.observedFingerprint != request.expectedCurrentFingerprint",
    "binding.observedState.recording",
    "binding.observedState.pending",
    "toggleShapeValid(",
    'operation.actionFamily = request.kind == NativeTimerModifyKind::update',
    "MutationOperationVerificationPolicy::readbackRequired",
    "reserveWithPayload(",
):
    require(preparation, needle, "fenced UPDATE/TOGGLE preparation")

for needle in (
    "observation.observedAt < expectation.readbackNotBefore",
    "nativeTimerObservationMatchesSpecification(",
    "current.bindingRevision != payload.expectedBindingRevision",
    "current.observedFingerprint != payload.expectedCurrentFingerprint",
    "next.lastVerifiedOperationId = expectation.operationId",
    "repository_.update(next, current.bindingRevision)",
):
    require(verification, needle, "authoritative PRESENT verification")

for needle in (
    "NativeTimerModifyKind::update",
    "NativeTimerModifyKind::toggle",
    "currentFingerprintConflict",
    "toggleShapeConflict",
    "alreadyVerified",
):
    require(test, needle, "UPDATE/TOGGLE regression")

require(doc, "No blind retry", "no-blind-retry documentation")
require(doc, "authoritative PRESENT", "readback documentation")
print("Phase 64 native Timer UPDATE/TOGGLE operation guard passed")
