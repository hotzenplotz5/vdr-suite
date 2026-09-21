#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

header_path = "core/timers/include/NativeTimerBindingRepository.h"
source_paths = [
    "core/timers/src/NativeTimerBindingRepository.cpp",
    "core/timers/src/NativeTimerBindingRepositoryStorage.h",
    "core/timers/src/NativeTimerBindingReadRepository.cpp",
    "core/timers/src/NativeTimerBindingWriteRepository.cpp",
]
test_path = "core/timers/tests/test_native_timer_binding_repository.cpp"
doc_path = "docs/development/phase-64-native-timer-binding-repository.md"
make_path = "mk/phase64-timer-intent-tests.mk"
required_files = [
    "core/timers/include/NativeTimerBinding.h",
    "core/timers/src/NativeTimerBinding.cpp",
    header_path,
    *source_paths,
    test_path,
    doc_path,
    make_path,
]
for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Phase-64 NativeTimerBinding repository file: {relative}")

header = (ROOT / header_path).read_text(encoding="utf-8")
source = "\n".join((ROOT / p).read_text(encoding="utf-8") for p in source_paths)
test = (ROOT / test_path).read_text(encoding="utf-8")
doc = (ROOT / doc_path).read_text(encoding="utf-8")
make_fragment = (ROOT / make_path).read_text(encoding="utf-8")

for token in [
    "NativeTimerBindingRepositoryStatus",
    "NativeTimerBindingRepositoryResult",
    "NativeTimerBindingRepositoryListResult",
    "class NativeTimerBindingRepository",
    "nativeIdentityConflict",
    "assignmentBindingConflict",
    "immutableConflict",
    "generationConflict",
    "observationConflict",
    "findByBackendNativeTimer",
    "listForAssignment",
    "expectedRevision",
]:
    if token not in header:
        raise SystemExit(f"missing NativeTimerBinding repository header marker: {token}")

for token in [
    "CREATE TABLE IF NOT EXISTS native_timer_bindings (",
    "idx_native_timer_bindings_native_identity",
    "idx_native_timer_bindings_managed_assignment",
    "BEGIN IMMEDIATE TRANSACTION;",
    "nativeTimerBindingRevisionMatches(",
    "next.backendNativeTimerId",
    "next.timerAssignmentId",
    "next.ownership != current.ownership",
    "next.backendGeneration < current.backendGeneration",
    "next.lastObservedAt < current.lastObservedAt",
    "WHERE native_timer_binding_id=? AND binding_revision=?;",
]:
    if token not in source:
        raise SystemExit(f"missing NativeTimerBinding repository source marker: {token}")

for token in [
    'database.open(":memory:")',
    'created.binding.bindingRevision == "1"',
    "NativeTimerBindingRepositoryStatus::alreadyExists",
    "NativeTimerBindingRepositoryStatus::nativeIdentityConflict",
    "NativeTimerBindingRepositoryStatus::assignmentBindingConflict",
    "NativeTimerBindingRepositoryStatus::immutableConflict",
    "NativeTimerBindingRepositoryStatus::generationConflict",
    "NativeTimerBindingRepositoryStatus::observationConflict",
    "listForAssignment",
    "databaseA.open(sharedPath)",
    "databaseB.open(sharedPath)",
    'writerBConflict.binding.bindingRevision == "2"',
]:
    if token not in test:
        raise SystemExit(f"missing NativeTimerBinding repository regression marker: {token}")

for token in [
    "NativeTimerBinding Persistence Repository",
    "sole issuer",
    "backendId + backendNativeTimerId",
    "at most one ownership in {managed, adopted}",
    "No implicit adoption",
    "BEGIN IMMEDIATE TRANSACTION",
    "two independent `Database` connections",
    "no installed runtime path",
    "native-observation mapping/readback boundary",
]:
    if token not in doc:
        raise SystemExit(f"missing Phase-64 Slice-10 repository statement: {token}")

for token in [
    "test-phase64-native-timer-binding-repository-architecture",
    "test-phase64-native-timer-binding-repository:",
    "core/timers/src/NativeTimerBindingRepository.cpp",
    "core/timers/src/NativeTimerBindingReadRepository.cpp",
    "core/timers/src/NativeTimerBindingWriteRepository.cpp",
    "core/timers/tests/test_native_timer_binding_repository.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make_fragment:
        raise SystemExit(f"missing NativeTimerBinding repository test-graph marker: {token}")

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
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc",
            ".mk", ".conf", ".service",
        }:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "NativeTimerBindingRepository" in text:
            raise SystemExit(
                "premature NativeTimerBinding repository runtime wiring: "
                + str(path.relative_to(ROOT)))

contract_text = header + source + test
for forbidden in [
    "TimerAssignmentSchedulingService", "TimerAssignmentPlanner",
    "BackendAgentCommand", "SuiteBridge", "SVDRP", "mutations=enabled",
]:
    if forbidden in contract_text:
        raise SystemExit(
            f"premature NativeTimerBinding repository boundary crossing: {forbidden}")

for relative in [header_path, *source_paths, test_path, make_path]:
    if "mutations=enabled" in (ROOT / relative).read_text(encoding="utf-8"):
        raise SystemExit(
            f"NativeTimerBinding repository slice must not enable mutations: {relative}")

print("Phase-64 NativeTimerBinding repository check passed")
print(
    "Slice-10 boundary: binding persistence/revision only; "
    "observation mapping/reconciliation/native mutation deferred")
