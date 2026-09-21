#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []

def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        errors.append(f"missing file: {path}")
        return ""
    return target.read_text(encoding="utf-8")

domain = read("core/vdr/include/LegacyOsdDomain.h")
snapshot = read("vdr-plugin-suite-bridge/suitebridge_osd_snapshot.h")
state_h = read("vdr-plugin-suite-bridge/suitebridge_osd_state.h")
state_cpp = read("vdr-plugin-suite-bridge/suitebridge_osd_state.cpp")
monitor_h = read("vdr-plugin-suite-bridge/suitebridge_status_monitor.h")
monitor_cpp = read("vdr-plugin-suite-bridge/suitebridge_status_monitor.cpp")
makefile = read("Makefile")
phase_make = read("mk/phase68-legacy-osd-tests.mk")
plugin_make = read("vdr-plugin-suite-bridge/Makefile")

for fragment in (
    "struct OsdSurfaceRef",
    "backendGeneration",
    "osdEpoch",
    "struct OsdFrame",
    "frameSequence",
    "fullFrame = true",
):
    if fragment not in domain:
        errors.append(f"Legacy OSD domain missing contract: {fragment}")

for fragment in (
    "MaximumItems = 128",
    "EpochHexLength = 32",
    "frameSequence",
    "observedAtMilliseconds",
    "droppedUpdates",
):
    if fragment not in snapshot:
        errors.append(f"SuiteBridge OSD snapshot missing bound/fence: {fragment}")

for fragment in (
    "std::atomic_flag gate_",
    "TryLock() const noexcept",
    "SuiteBridgeSaturatingCounter frameSequence_",
    "SuiteBridgeSaturatingCounter droppedUpdates_",
):
    if fragment not in state_h:
        errors.append(f"SuiteBridge OSD state missing nonblocking primitive: {fragment}")

for fragment in (
    "gate_.test_and_set",
    "MarkDroppedUpdate",
    "SuiteBridgeCounterEpoch epoch",
    "frameSequence_.Increment()",
    "state_.complete = false",
):
    if fragment not in state_cpp:
        errors.append(f"SuiteBridge OSD state missing continuity behavior: {fragment}")

for fragment in (
    "CaptureOsdSnapshot() const noexcept",
    "OsdClear(void)",
    "OsdTitle(const char *title)",
    "OsdStatusMessage(eMessageType type, const char *message)",
    "OsdItem(const char *text, int index, bool selectable)",
    "OsdCurrentItem(const char *text, int index)",
    "OsdTextItem(const char *text, bool scroll)",
    "OsdChannel(const char *text)",
    "OsdProgramme(",
):
    if fragment not in monitor_h:
        errors.append(f"SuiteBridge status monitor missing OSD callback: {fragment}")

for fragment in (
    "osdState_.Clear()",
    "osdState_.Title(title)",
    "osdState_.Item(text, index, selectable)",
    "osdState_.CurrentItem(text, index)",
    "osdState_.Channel(text)",
    "osdState_.Programme(",
):
    if fragment not in monitor_cpp:
        errors.append(f"SuiteBridge status monitor missing OSD wiring: {fragment}")

combined = "\n".join((state_h, state_cpp, monitor_h, monitor_cpp))
for forbidden in (
    "SVDRPCommand",
    "cRemote",
    "Put(",
    "socket(",
    "fork(",
    "execv(",
    "system(",
    "std::thread",
    "cThread",
):
    if forbidden in combined:
        errors.append(f"read-only OSD observation contains forbidden control/blocking surface: {forbidden}")

if "include mk/phase68-legacy-osd-tests.mk" not in makefile:
    errors.append("top-level Makefile must include Phase-68 test slice")
if "test-phase68-legacy-osd-observation" not in phase_make:
    errors.append("Phase-68 Make slice missing aggregate test")
if "suitebridge_osd_state.o" not in plugin_make:
    errors.append("SuiteBridge plugin build must compile OSD state")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("phase68 legacy osd observation contract ok")
