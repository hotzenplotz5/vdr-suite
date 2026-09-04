#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

context_path = ROOT / "core/daemon/include/BackendRuntimeContext.h"
make_path = ROOT / "mk/recording-native-editing-tests.mk"
api_header_path = ROOT / "api/rest/include/RecordingMarksApiRuntime.h"
api_source_path = ROOT / "api/rest/src/RecordingMarksApiRuntime.cpp"
router_path = ROOT / "api/rest/include/ApiRouter.h"
daemon_path = ROOT / "core/daemon/src/DaemonRuntime.cpp"

paths = (
    context_path,
    make_path,
    api_header_path,
    api_source_path,
    router_path,
    daemon_path,
)

errors = []
for path in paths:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

context = context_path.read_text(encoding="utf-8")
makefile = make_path.read_text(encoding="utf-8")
api_header = api_header_path.read_text(encoding="utf-8")
api_source = api_source_path.read_text(encoding="utf-8")
router = router_path.read_text(encoding="utf-8")
daemon = daemon_path.read_text(encoding="utf-8")

required_context = (
    '#include "SuiteBridgeRecordingMarksResolver.h"',
    "SuiteBridgeRecordingMarksResolver> recordingMarksResolver",
    "SuiteBridgeRecordingMarksResolver* ensureRecordingMarksResolver()",
    "if (!suiteBridgeTransport)",
    "if (!recordingMarksResolver)",
    "std::make_unique<SuiteBridgeRecordingMarksResolver>",
    "*suiteBridgeTransport",
    "return recordingMarksResolver.get();",
)

for fragment in required_context:
    if fragment not in context:
        errors.append(f"missing BackendRuntimeContext marks wiring: {fragment}")

required_make = (
    "VDR_RECORDING_NATIVE_MARKS_SRC :=",
    "core/vdr/src/SuiteBridgeRecordingMarksResolver.cpp",
    "RECORDING_NATIVE_EDITING_REST_SRC :=",
    "api/rest/src/RecordingMarksApiRuntime.cpp",
    "DAEMON_SRC += $(VDR_RECORDING_NATIVE_MARKS_SRC)",
    "DAEMON_SRC += $(RECORDING_NATIVE_EDITING_REST_SRC)",
    "REST_ROUTER_SRC += $(RECORDING_NATIVE_EDITING_REST_SRC)",
    "test-recording-marks-api-runtime:",
    "check-recording-native-editing-runtime-wiring:",
    "python3 tools/check_recording_native_editing_runtime_wiring.py",
)

for fragment in required_make:
    if fragment not in makefile:
        errors.append(f"missing native marks build wiring: {fragment}")

required_api = (
    '"/api/vdr/recordings/marks"',
    'key != "backend" && key != "recordingId"',
    "recording.id != request.recordingId",
    "recording.backendId != request.backendId",
    "VdrRecordingNativeIdentity::keyForNativeId(",
    "selected->backendNativeId",
    "access.resolver->resolve(recordingKey)",
    "recording_marks_capability_unavailable",
    "recording_native_state_stale",
    "recording_marks_invalid_native_payload",
    'response.headers["Cache-Control"] = "no-store"',
)

for fragment in required_api:
    if fragment not in api_source:
        errors.append(f"missing read-only Recording marks API contract: {fragment}")

required_router = (
    '#include "RecordingMarksApiRuntime.h"',
    "RecordingMarksApiRuntime::instance().tryHandleGet(",
)
for fragment in required_router:
    if fragment not in router:
        errors.append(f"missing Recording marks router wiring: {fragment}")

required_daemon = (
    '#include "RecordingMarksApiRuntime.h"',
    "RecordingMarksApiRuntime::instance().configure(",
    "findAllForBackend(",
    'capabilityAvailable(\n                            "recording-marks")',
    "ensureRecordingMarksResolver()",
    "RecordingMarksApiRuntime::instance().reset();",
)
for fragment in required_daemon:
    if fragment not in daemon:
        errors.append(f"missing Recording marks daemon wiring: {fragment}")

for forbidden in (
    "RecordingActionType::Cut",
    "NTCREATE",
    "NTMOD",
    "NTDELETE",
):
    if forbidden in context or forbidden in api_source or forbidden in api_header:
        errors.append(f"unexpected mutation wiring in read-only slice: {forbidden}")

for forbidden_query in (
    'key == "backendNativeId"',
    'key == "path"',
    'key == "recordingPath"',
):
    if forbidden_query in api_source:
        errors.append(f"client native-path authority exposed: {forbidden_query}")

if "RecordingMarksApiRuntime::instance().tryHandlePost" in router:
    errors.append("Recording marks mutation route opened during read-only slice")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("recording native editing runtime wiring ok")
