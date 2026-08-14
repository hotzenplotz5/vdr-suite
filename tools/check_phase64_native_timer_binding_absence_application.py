#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/timers/include/NativeTimerBindingAbsenceApplicationService.h"
SOURCE = ROOT / "core/timers/src/NativeTimerBindingAbsenceApplicationService.cpp"
TEST = ROOT / "core/timers/tests/test_native_timer_binding_absence_application_service.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-binding-absence-application.md"
MK = ROOT / "mk/phase64-native-timer-binding-absence-application-tests.mk"
MAKEFILE = ROOT / "Makefile"

for path in [HEADER, SOURCE, TEST, DOC, MK, MAKEFILE]:
    if not path.is_file():
        raise SystemExit(f"missing Slice-17 file: {path.relative_to(ROOT)}")

header = HEADER.read_text(encoding="utf-8")
source = SOURCE.read_text(encoding="utf-8")
test = TEST.read_text(encoding="utf-8")
doc = DOC.read_text(encoding="utf-8")
mk = MK.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")

for token in [
    "NativeTimerBindingAbsenceApplicationStatus",
    "missingRecorded",
    "missingRefreshed",
    "alreadyCurrent",
    "present",
    "reconciliationRequired",
    "bindingNotFound",
    "backendConflict",
    "staleGeneration",
    "staleEvidence",
    "repositoryConflict",
    "class NativeTimerBindingAbsenceApplicationService",
    "NativeTimerInventoryEvidence",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-17 header marker: {token}")

for token in [
    "nativeTimerInventoryEvidenceValid(evidence)",
    "repository_.findById(nativeTimerBindingId)",
    "evidence.backendId != current.backendId",
    "evidence.backendGeneration < current.backendGeneration",
    "evidence.observedAt < current.lastObservedAt",
    "assessNativeTimerAbsence(evidence, request)",
    "current.missingSince == 0",
    "next.missingSince = evidence.observedAt",
    "NativeTimerBindingDriftState::expectedTransition",
    "NativeTimerBindingDriftState::ambiguous",
    "repository_.update(next, current.bindingRevision)",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-17 source marker: {token}")

for forbidden in [
    "sqlite3_",
    "VdrTimer",
    "RestfulApi",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
    "NativeTimerBindingDriftState::externalDelete;",
]:
    if forbidden in header + source:
        raise SystemExit(f"premature Slice-17 boundary crossing: {forbidden}")

for token in [
    "NativeTimerBindingAbsenceApplicationStatus::missingRecorded",
    "missing.binding.missingSince == 2100",
    "NativeTimerBindingDriftState::ambiguous",
    "NativeTimerBindingAbsenceApplicationStatus::alreadyCurrent",
    "NativeTimerBindingAbsenceApplicationStatus::missingRefreshed",
    "NativeTimerBindingAbsenceApplicationStatus::present",
    "NativeTimerBindingAbsenceApplicationStatus::reconciliationRequired",
    "NativeTimerBindingOwnership::external",
    "NativeTimerBindingDriftState::expectedTransition",
    "NativeTimerBindingDriftState::externalDelete",
    "NativeTimerBindingAbsenceApplicationStatus::backendConflict",
    "NativeTimerBindingAbsenceApplicationStatus::staleGeneration",
    "NativeTimerBindingAbsenceApplicationStatus::staleEvidence",
    "test_native_timer_binding_absence_application_service passed",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-17 regression marker: {token}")

for token in [
    "NativeTimerBinding Absence Application",
    "authoritative missing",
    "observation evidence",
    "never invents",
    "`external_delete`",
    "observation fact is ownership-neutral",
    "existing `expected_transition` is preserved",
    "original `missingSince`",
    "no hidden retry",
    "expected absence",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-17 documentation marker: {token}")

for token in [
    "test-phase64-native-timer-binding-absence-application-architecture",
    "test-phase64-native-timer-binding-absence-application:",
    "core/timers/src/NativeTimerBindingAbsenceApplicationService.cpp",
    "core/timers/tests/test_native_timer_binding_absence_application_service.cpp",
    "test-fast: test-phase64-native-timer-binding-absence-application",
    "test-architecture: test-phase64-native-timer-binding-absence-application-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing Slice-17 make marker: {token}")

if "include mk/phase64-native-timer-binding-absence-application-tests.mk" not in makefile:
    raise SystemExit("Slice-17 make fragment is not included")

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
        if "NativeTimerBindingAbsenceApplicationService" in path.read_text(
            encoding="utf-8", errors="ignore"
        ):
            raise SystemExit(
                "premature Slice-17 runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 NativeTimerBinding absence application check passed")
print(
    "Slice-17 boundary: authoritative missing observation application only; "
    "cause classification, delete verification and native mutation deferred"
)
