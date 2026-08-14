#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/timers/include/NativeTimerInventoryEvidence.h"
SOURCE = ROOT / "core/timers/src/NativeTimerInventoryEvidence.cpp"
TEST = ROOT / "core/timers/tests/test_native_timer_inventory_evidence.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-inventory-evidence.md"
MK = ROOT / "mk/phase64-native-timer-inventory-evidence-tests.mk"
MAKEFILE = ROOT / "Makefile"

for path in [HEADER, SOURCE, TEST, DOC, MK, MAKEFILE]:
    if not path.is_file():
        raise SystemExit(f"missing Slice-15 file: {path.relative_to(ROOT)}")

header = HEADER.read_text(encoding="utf-8")
source = SOURCE.read_text(encoding="utf-8")
test = TEST.read_text(encoding="utf-8")
doc = DOC.read_text(encoding="utf-8")
mk = MK.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")

for token in [
    "NativeTimerInventoryCompleteness",
    "unknown",
    "complete",
    "NativeTimerInventoryEvidence",
    "backendGeneration",
    "backendNativeTimerIds",
    "NativeTimerAbsenceAssessmentRequest",
    "notBefore",
    "NativeTimerAbsenceAssessmentStatus",
    "absent",
    "present",
    "backendConflict",
    "generationConflict",
    "staleEvidence",
    "assessNativeTimerAbsence",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-15 header marker: {token}")

for token in [
    "kMaxIdentityLength = 160",
    "kMaxInventorySize = 4096",
    "evidence.completeness",
    "previous < id",
    "evidence.backendId != request.backendId",
    "evidence.backendGeneration != request.backendGeneration",
    "evidence.observedAt < request.notBefore",
    "std::binary_search",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-15 source marker: {token}")

# Reject real architectural dependencies, not explanatory comments that name
# legacy APIs while documenting why they are insufficient.
for forbidden in [
    '#include "VdrSnapshot.h"',
    '#include "VdrService.h"',
    '#include "VdrTimer.h"',
    "RestfulApiVdrAdapter",
    "RestfulApiTimerMapper",
    "NativeTimerBindingRepository",
    "sqlite3_",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in header + source + test:
        raise SystemExit(f"premature Slice-15 dependency: {forbidden}")

for token in [
    "NativeTimerAbsenceAssessmentStatus::present",
    "NativeTimerAbsenceAssessmentStatus::absent",
    "NativeTimerInventoryCompleteness::unknown",
    "duplicate",
    "unsorted",
    "backendConflict",
    "generationConflict",
    "staleEvidence",
    "std::string(160, 'a')",
    "std::string(161, 'a')",
    "4097",
    "test_native_timer_inventory_evidence passed",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-15 regression marker: {token}")

for token in [
    "Authoritative Native Timer Inventory Evidence",
    "empty vector",
    "explicit `NativeTimerInventoryCompleteness::complete`",
    "interpreted as proof that a native Timer was deleted",
    "transport/request succeeded",
    "payload parsing succeeded",
    "pagination/coverage is complete",
    "new explicit result/envelope",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-15 documentation marker: {token}")

for token in [
    "test-phase64-native-timer-inventory-evidence-architecture",
    "test-phase64-native-timer-inventory-evidence:",
    "core/timers/src/NativeTimerInventoryEvidence.cpp",
    "core/timers/tests/test_native_timer_inventory_evidence.cpp",
    "test-fast: test-phase64-native-timer-inventory-evidence",
    "test-architecture: test-phase64-native-timer-inventory-evidence-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing Slice-15 make marker: {token}")

include_line = "include mk/phase64-native-timer-inventory-evidence-tests.mk"
if include_line not in makefile:
    raise SystemExit("Slice-15 make fragment is not included")

# Slice 16 adds one explicitly reviewed, test-only provider-side producer for this
# evidence. Keep the older Slice-15 runtime-wiring fence closed everywhere else.
allowed_later_consumers = {
    "core/vdr/include/RestfulApiNativeTimerInventoryReader.h",
    "core/vdr/src/RestfulApiNativeTimerInventoryReader.cpp",
    "core/vdr/tests/test_restfulapi_native_timer_inventory_reader.cpp",
}

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
        relative = str(path.relative_to(ROOT))
        if "NativeTimerInventoryEvidence" in path.read_text(
            encoding="utf-8", errors="ignore"
        ) and relative not in allowed_later_consumers:
            raise SystemExit(
                "premature Slice-15 runtime wiring: " + relative
            )

print("Phase-64 native Timer inventory evidence check passed")
print(
    "Slice-15 boundary: complete backend-generation-fenced inventory evidence "
    "only; producer, missing-state application and native mutation deferred"
)
