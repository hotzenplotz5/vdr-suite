#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/TimerAssignment.h",
    "core/timers/src/TimerAssignment.cpp",
    "core/timers/include/TimerAssignmentRepository.h",
    "core/timers/src/TimerAssignmentRepository.cpp",
    "core/timers/tests/test_timer_assignment_repository.cpp",
    "docs/development/phase-64-timer-assignment-repository.md",
    "mk/phase64-timer-intent-tests.mk",
]

for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(
            f"missing Phase-64 TimerAssignment repository file: {relative}")

header = (ROOT / required_files[2]).read_text(encoding="utf-8")
source = (ROOT / required_files[3]).read_text(encoding="utf-8")
test = (ROOT / required_files[4]).read_text(encoding="utf-8")
doc = (ROOT / required_files[5]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[6]).read_text(encoding="utf-8")

for token in [
    "TimerAssignmentRepositoryStatus",
    "TimerAssignmentRepositoryResult",
    "TimerAssignmentRepositoryListResult",
    "class TimerAssignmentRepository",
    "ownershipConflict",
    "intentNotFound",
    "intentRevisionConflict",
    "findActivePrimaryForIntent",
    "listForIntent",
    "expectedRevision",
]:
    if token not in header:
        raise SystemExit(
            f"missing TimerAssignment repository header marker: {token}")

for token in [
    '"CREATE TABLE IF NOT EXISTS timer_assignments ("',
    '"UNIQUE(timer_intent_id,assignment_epoch),"',
    '"idx_timer_assignments_active_exclusive_owner "',
    '"BEGIN IMMEDIATE TRANSACTION;"',
    "nextAssignmentEpoch(",
    "validateIntentRevision(",
    "timerAssignmentActiveOwnershipState(durable.state)",
    "timerAssignmentRevisionMatches(",
    "timerAssignmentTerminal(current.state)",
    "timerAssignmentCanTransition(",
    "!current.backendId.empty()",
    "TimerAssignmentRepositoryStatus::ownershipConflict",
    '"AND assignment_revision=?;"',
]:
    if token not in source:
        raise SystemExit(
            f"missing TimerAssignment repository source marker: {token}")

for token in [
    'database.open(":memory:")',
    'created.assignment.assignmentRevision == "1"',
    "created.assignment.assignmentEpoch == 1",
    "TimerAssignmentRepositoryStatus::alreadyExists",
    "TimerAssignmentRepositoryStatus::intentRevisionConflict",
    "TimerAssignmentRepositoryStatus::intentNotFound",
    "TimerAssignmentRepositoryStatus::ownershipConflict",
    'conflict.assignment.assignmentRevision == "2"',
    'nativeTimerBindingId = "native-timer-binding:1"',
    'SET intent_revision=4',
    "TimerAssignmentRole::replica",
    "TimerAssignmentRepositoryStatus::notFound",
]:
    if token not in test:
        raise SystemExit(
            f"missing TimerAssignment repository regression marker: {token}")

for token in [
    "TimerAssignment Persistence and Repository Semantics",
    "issues both revision and assignment epoch",
    "assignmentEpoch",
    "exact parent TimerIntent revision",
    "single active primary",
    "selected backend identity",
    "Terminal assignments",
    "BEGIN IMMEDIATE",
    "no scheduler",
    "mutations=enabled",
    "no real yaVDR acceptance",
]:
    if token not in doc:
        raise SystemExit(
            f"missing Phase-64 Slice-4 repository statement: {token}")

for token in [
    "test-phase64-timer-assignment-repository-architecture",
    "test-phase64-timer-assignment-repository:",
    "core/timers/src/TimerAssignmentRepository.cpp",
    "core/timers/tests/test_timer_assignment_repository.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make_fragment:
        raise SystemExit(
            f"missing TimerAssignment repository test-graph marker: {token}")

forbidden_roots = [
    ROOT / "apps",
    ROOT / "api",
    ROOT / "core" / "agent",
    ROOT / "core" / "daemon",
    ROOT / "core" / "http",
    ROOT / "core" / "runtime",
    ROOT / "core" / "vdr",
    ROOT / "vdr-plugin-suite-bridge",
]
text_suffixes = {
    ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc", ".mk",
    ".conf", ".service",
}
readback_successor_guard = (
    ROOT / "tools/check_phase69_native_timer_create_readback_reconciliation.py"
)
if not readback_successor_guard.is_file():
    raise SystemExit(
        "Phase-69.C readback-reconciliation guard is required for the reviewed "
        "TimerAssignment repository regression consumer"
    )
readback_successor = readback_successor_guard.read_text(encoding="utf-8")
productive_successor_guard = (
    ROOT / "tools/check_phase69_native_timer_create_productive_runtime.py"
)
if not productive_successor_guard.is_file():
    raise SystemExit(
        "Phase-69.C productive Timer CREATE guard is required for the "
        "reviewed TimerAssignment repository runtime consumer"
    )
productive_successor = productive_successor_guard.read_text(encoding="utf-8")
if "core/daemon/tests/test_native_timer_create_readback_reconciliation.cpp" not in readback_successor:
    raise SystemExit(
        "Phase-69.C readback-reconciliation guard must pin its repository "
        "regression consumer"
    )

reviewed_runtime_files = {
    Path("core/daemon/include/DaemonRuntime.h"),
    Path("core/daemon/src/DaemonRuntimeInitialization.cpp"),
    Path("core/daemon/tests/test_native_timer_create_readback_reconciliation.cpp"),
    Path("core/daemon/tests/test_native_timer_create_productive_runtime.cpp"),
}
for marker in [
    "core/daemon/tests/test_native_timer_create_productive_runtime.cpp",
    "MutationOperationState::succeeded",
    "TimerAssignmentState::bound",
]:
    if marker not in productive_successor:
        raise SystemExit(
            "productive Timer CREATE guard missing TimerAssignment repository "
            "consumer marker: " + marker
        )

for reviewed in reviewed_runtime_files:
    if not (ROOT / reviewed).is_file():
        raise SystemExit(
            f"missing reviewed TimerAssignment repository runtime file: {reviewed}")

for root in forbidden_roots:
    if not root.exists():
        continue
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in text_suffixes:
            continue
        relative = path.relative_to(ROOT)
        if relative in reviewed_runtime_files:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "TimerAssignmentRepository" in text:
            raise SystemExit(
                "premature TimerAssignment repository runtime wiring: "
                + str(relative))

for relative in [
    required_files[2],
    required_files[3],
    required_files[4],
    required_files[6],
]:
    text = (ROOT / relative).read_text(encoding="utf-8")
    if "mutations=enabled" in text:
        raise SystemExit(
            f"TimerAssignment repository slice must not enable mutations: {relative}")

print("Phase-64 TimerAssignment repository check passed")
print(
    "Slice-4 boundary: assignment persistence only; "
    "scheduler/native runtime deferred")
