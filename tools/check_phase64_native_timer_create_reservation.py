#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/agent/include/BackendAgentNativeTimerCreateReservation.h": [
        "BackendAgentNativeTimerCreateReservationRequest",
        "expectedAssignmentRevision",
        "expectedIntentRevision",
        "assignmentEpoch",
        "nativeTimerBindingId",
        "expectedSpecificationFingerprint",
        "BackendAgentNativeTimerCreateReservationService",
    ],
    "core/agent/src/BackendAgentNativeTimerCreateReservation.cpp": [
        "reservationRepository_.findForOperation",
        "reservationRepository_.reserve(assignment, &*selection)",
        "native_timer_create_provider_selection_stale",
        "native_timer_create_active_assignment_conflict",
        "kBackendAgentNativeTimerCreateCapability",
    ],
    "core/agent/tests/test_backend_agent_native_timer_create_reservation.cpp": [
        "Reservation is durable but must remain invisible",
        "native_timer_create_reservation_replayed",
        "native_timer_create_provider_selection_stale",
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

source = (ROOT / "core/agent/src/BackendAgentNativeTimerCreateReservation.cpp").read_text(
    encoding="utf-8"
)
if "insertAssignment(" in source:
    errors.append("CREATE reservation must not directly activate/poll an Agent assignment")
if ".activate(" in source:
    errors.append("CREATE reservation must not activate its command reservation")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-reservation-tests.mk" not in makefile:
    errors.append("Makefile: missing CREATE reservation tests include")

if errors:
    for error in errors:
        print(error)
    raise SystemExit(1)
print("Phase 64 native Timer CREATE reservation architecture guard passed")
