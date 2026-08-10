#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/TimerAssignmentRepository.h",
    "core/timers/src/TimerAssignmentSetRevisionRepository.cpp",
    "core/timers/tests/test_timer_assignment_set_revision_repository.cpp",
    "docs/development/phase-64-timer-assignment-set-revision-fence.md",
    "mk/phase64-timer-intent-tests.mk",
]

for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(
            f"missing Phase-64 assignment-set revision file: {relative}"
        )

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[4]).read_text(encoding="utf-8")

for token in [
    "TimerAssignmentRepositorySetRevisionResult",
    "assignmentSetRevisionForIntent",
    "createAgainstAssignmentSetRevision",
    "expectedAssignmentSetRevision",
]:
    if token not in header:
        raise SystemExit(
            f"missing assignment-set repository header marker: {token}"
        )

for token in [
    "timer_assignment_set_revisions",
    "timer_assignment_set_expectations",
    "trg_timer_assignment_set_expectation_insert",
    "trg_timer_assignment_set_revision_insert",
    "trg_timer_assignment_set_revision_update",
    "trg_timer_assignment_set_revision_delete",
    "BEFORE INSERT ON timer_assignments",
    "AFTER INSERT ON timer_assignments",
    "AFTER UPDATE ON timer_assignments",
    "AFTER DELETE ON timer_assignments",
    "timer_assignment_set_revision_conflict",
    "sqlite3_last_insert_rowid",
    "create(assignment)",
    "TimerAssignmentRepositoryStatus::conflict",
]:
    if token not in source:
        raise SystemExit(
            f"missing assignment-set repository source marker: {token}"
        )

for token in [
    'emptySet.assignmentSetRevision == "0"',
    'afterPrimary.assignmentSetRevision == "1"',
    "TimerAssignmentRepositoryStatus::conflict",
    "afterStaleAttempt.assignmentSetRevision == \"1\"",
    "sharedPlanningFence",
    "firstPlannedCreate.ok()",
    'afterCompetingPlans.assignmentSetRevision == "5"',
    '"not-a-revision"',
    "bootstrapped.assignmentSetRevision == \"1\"",
    "afterBootstrappedUpdate.assignmentSetRevision == \"2\"",
]:
    if token not in test:
        raise SystemExit(
            f"missing assignment-set revision regression marker: {token}"
        )

for token in [
    "assignment-set revision",
    "assignmentSetRevision",
    "does not replace `assignmentEpoch`",
    "read assignmentSetRevision",
    "list current assignments",
    "createAgainstAssignmentSetRevision",
    "BEFORE INSERT",
    "another process or another database connection",
    "repository-only",
    "replica persistence",
    "Replacement remains separate",
]:
    if token not in doc:
        raise SystemExit(
            f"missing Phase-64 Slice-7 statement: {token}"
        )

for token in [
    "test-phase64-timer-assignment-set-revision-architecture",
    "test-phase64-timer-assignment-set-revision:",
    "TimerAssignmentSetRevisionRepository.cpp",
    "test_timer_assignment_set_revision_repository.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make_fragment:
        raise SystemExit(
            f"missing assignment-set test-graph marker: {token}"
        )

# This slice is a persistence concurrency primitive only. SQLite is expected
# inside the Repository implementation, but scheduler/runtime/native boundaries
# remain forbidden.
for forbidden in [
    "TimerAssignmentPlanner",
    "TimerAssignmentSchedulingService",
    "NativeTimerBinding",
    "BackendAgentCommand",
    "SuiteBridge",
    "RESTfulAPI",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in source + test:
        raise SystemExit(
            f"premature assignment-set boundary crossing: {forbidden}"
        )

runtime_roots = [
    ROOT / "apps",
    ROOT / "api",
    ROOT / "core" / "agent",
    ROOT / "core" / "daemon",
    ROOT / "core" / "http",
    ROOT / "core" / "runtime",
    ROOT / "core" / "vdr",
    ROOT / "vdr-plugin-suite-bridge",
]
source_suffixes = {
    ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc", ".mk",
    ".conf", ".service",
}
for root in runtime_roots:
    if not root.exists():
        continue
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in source_suffixes:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if (
            "assignmentSetRevisionForIntent" in text
            or "createAgainstAssignmentSetRevision" in text
        ):
            raise SystemExit(
                "premature assignment-set runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 TimerAssignment set-revision fence check passed")
print(
    "Slice-7 boundary: repository concurrency primitive only; "
    "replica/replacement scheduling deferred"
)
