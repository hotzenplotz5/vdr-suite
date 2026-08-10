#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/TimerAssignmentPlanner.h",
    "core/timers/src/TimerAssignmentPlanner.cpp",
    "core/timers/tests/test_timer_assignment_planner.cpp",
    "docs/development/phase-64-timer-assignment-planning.md",
    "mk/phase64-timer-intent-tests.mk",
]

for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(
            f"missing Phase-64 TimerAssignment planner file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[4]).read_text(encoding="utf-8")

for token in [
    "TimerAssignmentPlanningBackendState",
    "TimerAssignmentPlanningCapabilityEvidence",
    "TimerAssignmentPlanningHealthEvidence",
    "TimerAssignmentPlanningChannelEvidence",
    "TimerAssignmentPlanningBackendCandidate",
    "TimerAssignmentPlanningCandidateEvaluation",
    "TimerAssignmentPlanningDecision",
    "timerAssignmentPlanningPolicyVersion",
    "planTimerAssignment",
    "timerAssignmentPlanningDecisionEquivalent",
    "executionAuthorityFence",
]:
    if token not in header:
        raise SystemExit(
            f"missing TimerAssignment planner header marker: {token}")

for token in [
    '"timer-assignment-planner/1"',
    '"excluded_by_intent"',
    '"backend_write_forbidden"',
    '"execution_authority_unavailable"',
    '"capability_generation_stale"',
    '"timer_capability_missing"',
    '"health_missing_or_stale"',
    '"channel_mapping_missing_or_stale"',
    '"channel_mapping_generation_stale"',
    '"active_primary_exists"',
    '"backend_diversity_required"',
    '"site_diversity_required"',
    '"no_eligible_backend"',
    "std::sort(",
    "left->backendId < right->backendId",
    "selectedEvaluation.preferenceRank",
]:
    if token not in source:
        raise SystemExit(
            f"missing TimerAssignment planner source marker: {token}")

for token in [
    "timerAssignmentPlanningDecisionEquivalent(first, second)",
    "TimerAssignmentPlanningBackendState::offline",
    "TimerAssignmentPlanningBackendState::stale",
    "TimerAssignmentPlanningBackendState::incompatible",
    "capability.backendGeneration = 3",
    "capability.timerCreate = false",
    "health.current = false",
    "channel.current = false",
    "channel.backendGeneration = 3",
    "executionAuthorityCurrent = false",
    "active_primary_exists",
    "TimerAssignmentRole::replica",
    "requireBackendDiversity = true",
    "requireSiteDiversity = true",
    "preferred_backends_ineligible",
    "decision.decisionEvidence.exclusions.size() == 32",
]:
    if token not in test:
        raise SystemExit(
            f"missing TimerAssignment planner regression marker: {token}")

for token in [
    "Deterministic TimerAssignment Planning Contract",
    "BackendNode.online",
    "executionAuthorityFence",
    "selection or fallback API",
    "preferredBackendIds",
    "stable lexical `backendId`",
    "decisionScore = -preferenceRank",
    "active_primary_exists",
    "single-active-primary",
    "assignmentEpoch",
    "no installed runtime path",
    "No real yaVDR acceptance is required",
]:
    if token not in doc:
        raise SystemExit(
            f"missing Phase-64 Slice-5 planning statement: {token}")

for token in [
    "test-phase64-timer-assignment-planner-architecture",
    "test-phase64-timer-assignment-planner:",
    "core/timers/src/TimerAssignmentPlanner.cpp",
    "core/timers/tests/test_timer_assignment_planner.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make_fragment:
        raise SystemExit(
            f"missing TimerAssignment planner test-graph marker: {token}")

planner_text = header + source + test
for forbidden in [
    "#include <sqlite3.h>",
    "sqlite3_",
    "TimerAssignmentRepository",
    "NativeTimerBindingRepository",
    "BackendAgentCommand",
    "SuiteBridge",
    "RESTfulAPI",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in planner_text:
        raise SystemExit(
            f"premature TimerAssignment planner boundary crossing: {forbidden}")

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
for root in forbidden_roots:
    if not root.exists():
        continue
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in text_suffixes:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "TimerAssignmentPlanner" in text:
            raise SystemExit(
                "premature TimerAssignment planner runtime wiring: "
                + str(path.relative_to(ROOT)))

print("Phase-64 TimerAssignment planner check passed")
print(
    "Slice-5 boundary: deterministic planning contract only; "
    "persistence handoff/native runtime deferred")
