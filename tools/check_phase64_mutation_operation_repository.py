#!/usr/bin/env python3
"""Architecture guard for Phase 64 Slice 20 shared mutation operation persistence."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
DOMAIN_H = ROOT / "core/operations/include/MutationOperation.h"
DOMAIN_CPP = ROOT / "core/operations/src/MutationOperation.cpp"
REPO_H = ROOT / "core/operations/include/MutationOperationRepository.h"
REPO_CPP = ROOT / "core/operations/src/MutationOperationRepository.cpp"
TEST = ROOT / "core/operations/tests/test_mutation_operation_repository.cpp"
DOC = ROOT / "docs/development/phase-64-mutation-operation-repository.md"
FRAGMENT = ROOT / "mk/phase64-mutation-operation-repository-tests.mk"
MAKEFILE = ROOT / "Makefile"

failures: list[str] = []


def require(path: Path, markers: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing Slice-20 file: {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            failures.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")


require(
    DOMAIN_H,
    [
        "enum class MutationOperationState",
        "executedUnverified",
        "outcomeUnknown",
        "MutationOperationVerificationPolicy",
        "operationId",
        "idempotencyKey",
        "backendGeneration",
        "expectedRevision",
        "actionFamily",
        "requestFingerprint",
        "operationRevision",
        "mutationOperationTransitionAllowed",
    ],
)
require(
    DOMAIN_CPP,
    [
        'return "accepted"',
        'return "rejected"',
        'return "conflict"',
        'return "queued"',
        'return "dispatching"',
        'return "executed_unverified"',
        'return "succeeded"',
        'return "failed_before_dispatch"',
        'return "failed_verified"',
        'return "outcome_unknown"',
        'return "cancelled"',
        'return "readback_required"',
    ],
)
require(
    REPO_H,
    [
        "class MutationOperationRepository",
        "reserve(",
        "findById(",
        "findByIdempotencyScope(",
        "transition(",
        "idempotentReplay",
        "idempotencyConflict",
        "revisionConflict",
        "stateConflict",
    ],
)
require(
    REPO_CPP,
    [
        "CREATE TABLE IF NOT EXISTS mutation_operations",
        "idx_mutation_operations_idempotency_scope",
        "actor_id,backend_id,resource_type,resource_id,action_family,idempotency_key",
        "BEGIN IMMEDIATE TRANSACTION",
        "operation_revision=?",
        "mutationOperationTransitionAllowed",
    ],
)
require(
    TEST,
    [
        "idempotentReplay",
        "idempotencyConflict",
        "operationConflict",
        "revisionConflict",
        "executedUnverified",
        "outcomeUnknown",
        "succeeded",
        "stateConflict",
    ],
)
require(
    DOC,
    [
        "ADR-0042",
        "no existing durable generic operation repository",
        "Single lifecycle authority",
        "Slice 21",
        "No Timer-specific operation table",
        "no daemon/runtime wiring",
    ],
)
require(
    FRAGMENT,
    [
        "test-phase64-mutation-operation-repository",
        "core/operations/src/MutationOperation.cpp",
        "core/operations/src/MutationOperationRepository.cpp",
        "core/operations/tests/test_mutation_operation_repository.cpp",
        "test-fast: test-phase64-mutation-operation-repository",
        "test-architecture: test-phase64-mutation-operation-repository-architecture",
    ],
)
require(
    MAKEFILE,
    ["include mk/phase64-mutation-operation-repository-tests.mk"],
)

production = ""
for path in (DOMAIN_H, DOMAIN_CPP, REPO_H, REPO_CPP):
    if path.is_file():
        production += path.read_text(encoding="utf-8") + "\n"

for forbidden in [
    "NativeTimer",
    "TimerAssignment",
    "BackendAgent",
    "RestfulApi",
    "SuiteBridge",
    "SVDRP",
    "DaemonRuntime",
    "core/timers",
    "core/agent",
    "core/vdr",
]:
    if forbidden in production:
        failures.append(f"generic operation authority contains forbidden domain/runtime coupling: {forbidden}")

for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "MutationOperation.cpp" in text or "MutationOperationRepository.cpp" in text:
        failures.append(
            f"Slice-20 operation source is unexpectedly wired into runtime sources: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 mutation operation repository architecture check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 mutation operation repository architecture check passed")
