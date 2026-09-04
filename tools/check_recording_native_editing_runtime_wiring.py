#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

context_path = ROOT / "core/daemon/include/BackendRuntimeContext.h"
make_path = ROOT / "mk/recording-native-editing-tests.mk"

errors = []
for path in (context_path, make_path):
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

context = context_path.read_text(encoding="utf-8")
makefile = make_path.read_text(encoding="utf-8")

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
    "DAEMON_SRC += $(VDR_RECORDING_NATIVE_MARKS_SRC)",
    "check-recording-native-editing-runtime-wiring:",
    "python3 tools/check_recording_native_editing_runtime_wiring.py",
)

for fragment in required_make:
    if fragment not in makefile:
        errors.append(f"missing native marks build wiring: {fragment}")

for forbidden in (
    "RecordingActionType::Cut",
    "NTCREATE",
    "NTMOD",
    "NTDELETE",
):
    if forbidden in context:
        errors.append(f"unexpected mutation wiring in read-only context: {forbidden}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("recording native editing runtime wiring ok")
