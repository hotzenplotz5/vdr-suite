#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/vdr/include/RestfulApiNativeTimerInventoryReader.h"
SOURCE = ROOT / "core/vdr/src/RestfulApiNativeTimerInventoryReader.cpp"
TEST = ROOT / "core/vdr/tests/test_restfulapi_native_timer_inventory_reader.cpp"
DOC = ROOT / "docs/development/phase-64-restfulapi-native-timer-inventory-reader.md"
MK = ROOT / "mk/phase64-restfulapi-native-timer-inventory-reader-tests.mk"
MAKEFILE = ROOT / "Makefile"
VDR_SOURCES = ROOT / "mk/vdr-sources.mk"

for path in [HEADER, SOURCE, TEST, DOC, MK, MAKEFILE, VDR_SOURCES]:
    if not path.is_file():
        raise SystemExit(f"missing Slice-16 file: {path.relative_to(ROOT)}")

header = HEADER.read_text(encoding="utf-8")
source = SOURCE.read_text(encoding="utf-8")
test = TEST.read_text(encoding="utf-8")
doc = DOC.read_text(encoding="utf-8")
mk = MK.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")
vdr_sources = VDR_SOURCES.read_text(encoding="utf-8")

for token in [
    "RestfulApiNativeTimerInventoryReadStatus",
    "complete",
    "invalidRequest",
    "httpError",
    "parseError",
    "RestfulApiNativeTimerInventoryReadRequest",
    "backendGeneration",
    "observedAt",
    "RestfulApiNativeTimerInventoryReadResult",
    "NativeTimerInventoryEvidence",
    "class RestfulApiNativeTimerInventoryReader",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-16 header marker: {token}")

for token in [
    'httpRequest.method = "GET"',
    'httpRequest.url = "/timers.json"',
    'httpRequest.headers["Accept"] = "application/json"',
    "response.statusCode != 200",
    "kMaxInventorySize = 4096",
    "kMaxPayloadLength = 4 * 1024 * 1024",
    "kMaxJsonDepth = 64",
    "parseCompleteTimerIds",
    "!keys.insert(key).second",
    "!parseTimerObject(cursor, id)",
    "std::sort(ids.begin(), ids.end())",
    "std::adjacent_find(ids.begin(), ids.end())",
    "NativeTimerInventoryCompleteness::complete",
    "nativeTimerInventoryEvidenceValid(value.evidence)",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-16 source marker: {token}")

for forbidden in [
    "RestfulApiTimerMapper::parseTimers",
    "NativeTimerBindingRepository",
    "sqlite3_",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in header + source + test:
        raise SystemExit(f"premature Slice-16 dependency: {forbidden}")

for token in [
    'R"({"timers":[]})"',
    "RestfulApiNativeTimerInventoryReadStatus::httpError",
    "RestfulApiNativeTimerInventoryReadStatus::parseError",
    'R"({"count":0})"',
    'missing id',
    'R"({"timers":[{"id":"17"},{"id":"17"}]})"',
    "4 * 1024 * 1024 + 1",
    "http.calls == beforeInvalid",
    "test_restfulapi_native_timer_inventory_reader passed",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-16 regression marker: {token}")

for token in [
    "Failure-aware RESTfulAPI Native Timer Inventory Reader",
    "HTTP success alone",
    "dedicated fail-closed JSON scanner",
    "every Timer object",
    "caps payload size at 4 MiB",
    "does not derive `backendGeneration` from `VdrSnapshot`",
    "intentionally **not**",
    "remain unchanged",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-16 documentation marker: {token}")

for token in [
    "test-phase64-restfulapi-native-timer-inventory-reader-architecture",
    "test-phase64-restfulapi-native-timer-inventory-reader:",
    "core/vdr/src/RestfulApiNativeTimerInventoryReader.cpp",
    "core/vdr/tests/test_restfulapi_native_timer_inventory_reader.cpp",
    "test-fast: test-phase64-restfulapi-native-timer-inventory-reader",
    "test-architecture: test-phase64-restfulapi-native-timer-inventory-reader-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing Slice-16 make marker: {token}")

if "include mk/phase64-restfulapi-native-timer-inventory-reader-tests.mk" not in makefile:
    raise SystemExit("Slice-16 make fragment is not included")
if "RestfulApiNativeTimerInventoryReader.cpp" in vdr_sources:
    raise SystemExit("Slice-16 reader is prematurely linked into the daemon VDR source list")

productive_successor_guard = (
    ROOT / "tools/check_phase69_native_timer_create_productive_runtime.py"
)
if not productive_successor_guard.is_file():
    raise SystemExit(
        "Phase-69.C productive Timer CREATE guard is required before the "
        "complete inventory reader may be wired into daemon polling"
    )
productive_successor = productive_successor_guard.read_text(encoding="utf-8")
for token in [
    "RestfulApiNativeTimerInventoryReader",
    "VdrManagedTimerCreateReadbackEvidenceBuilder::build(",
    "agent->backendGeneration != expectation.backendGeneration",
]:
    if token not in productive_successor:
        raise SystemExit(
            "productive Timer CREATE guard missing inventory-reader marker: "
            + token
        )
reviewed_productive_paths = {
    Path("core/daemon/src/DaemonRuntimePolling.cpp"),
}

for scan_root in [
    ROOT / "apps", ROOT / "api", ROOT / "core" / "agent",
    ROOT / "core" / "daemon", ROOT / "core" / "runtime",
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
        if "RestfulApiNativeTimerInventoryReader" in path.read_text(
            encoding="utf-8", errors="ignore"
        ):
            relative = path.relative_to(ROOT)
            if relative in reviewed_productive_paths:
                continue
            raise SystemExit(
                "premature Slice-16 runtime wiring: "
                + str(relative)
            )

print("Phase-64 RESTfulAPI native Timer inventory reader check passed")
print(
    "Slice-16 boundary: failure-aware complete inventory producer only; "
    "daemon wiring, missing-state application and native mutation deferred"
)
