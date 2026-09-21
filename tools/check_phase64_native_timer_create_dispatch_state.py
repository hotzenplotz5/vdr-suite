#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/timers/include/NativeTimerCreateDispatchService.h": [
        "NativeTimerCreateCommandReservationReference",
        "NativeTimerCreateDispatchClaimRequest",
        "claimAfterReservation",
        "alreadyClaimed",
    ],
    "core/timers/src/NativeTimerCreateDispatchService.cpp": [
        'native-timer-create-command-reservation/1|',
        'MutationOperationState::accepted',
        'MutationOperationState::dispatching',
        'parseNativeTimerCreateCommandReservationReference',
        'native.timer.create',
        'operation.expectedResourceFingerprint',
    ],
    "core/timers/tests/test_native_timer_create_dispatch_service.cpp": [
        "must never accept a fresh command identity",
        "operationRevision == \"2\"",
        "deadlineExpired",
    ],
}

errors = []
for relative, markers in required.items():
    path = ROOT / relative
    if not path.exists():
        errors.append(f"missing {relative}")
        continue
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            errors.append(f"{relative}: missing marker {marker}")

source = (ROOT / "core/timers/src/NativeTimerCreateDispatchService.cpp").read_text(
    encoding="utf-8"
)
if "BackendAgent" in source:
    errors.append("timer dispatch state must stay backend/Agent neutral")
if "insertAssignment(" in source or ".activate(" in source:
    errors.append("timer dispatch state must not activate an Agent command")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-dispatch-state-tests.mk" not in makefile:
    errors.append("Makefile: missing CREATE dispatch-state tests include")

if errors:
    for error in errors:
        print(error)
    raise SystemExit(1)
print("Phase 64 native Timer CREATE dispatch-state architecture guard passed")
