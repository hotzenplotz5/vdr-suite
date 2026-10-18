#!/usr/bin/env python3
"""Fail-closed architecture check, NOT a substitute for real VDR compilation."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
native = (ROOT / "vdr-plugin-suite-bridge/suitebridge_native_timer_preview_vdr.cpp").read_text(encoding="utf-8")
core = (ROOT / "vdr-plugin-suite-bridge/suitebridge_native_timer_preview_core.cpp").read_text(encoding="utf-8")
header = (ROOT / "vdr-plugin-suite-bridge/suitebridge_native_timer_preview_vdr.h").read_text(encoding="utf-8")
makefile = (ROOT / "vdr-plugin-suite-bridge/Makefile").read_text(encoding="utf-8")
tests = (ROOT / "vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_preview_core.cpp").read_text(encoding="utf-8")

for needle in (
    "GetTimersRead(", "GetChannelsRead(", "GetDevice(", "Query=true",
    "cDevice::GetDevice(", "timerKey.Remove()", "timerKey.TimedOut()",
    "validLocalProjection(", "cDevice::NumDevices()", "tfActive",
    "t->WeekDays() == 0", "t->Local()", "CA_FTA",
):
    if needle not in native:
        raise SystemExit(f"SLICE2_PREVIEW_READ_BOUNDARY=FAIL missing {needle}")

for needle in (
    "GetTimersWrite", "GetChannelsWrite", "SetModified(", "SetExplicitModify(",
    "SwitchChannel(", "AttachReceiver(", "Detach(", "timers->Add(",
    "timers->Del(", "SetChannel(", "EXEC vdr-suite-native",
):
    if needle in native:
        raise SystemExit(f"SLICE2_PREVIEW_READ_BOUNDARY=FAIL write path {needle}")

for needle in (
    "active_timer_overlap_not_simulated",
    "active_timer_schedule_indeterminate",
    "native_snapshot_incomplete",
    "device_inventory_incomplete",
    "no_proven_nonpreempting_native_device",
):
    if needle not in core:
        raise SystemExit(f"SLICE2_PREVIEW_FAIL_CLOSED=FAIL missing {needle}")

if "SuiteBridgeCaptureNativeTimerPreviewVdr" not in header:
    raise SystemExit("SLICE2_PREVIEW_CAPTURE=FAIL")
if "suitebridge_native_timer_preview_vdr.o" not in makefile:
    raise SystemExit("SLICE2_PREVIEW_VDR_LINK=FAIL")
if tests.count("Disposition::unknown") < 9:
    raise SystemExit("SLICE2_PREVIEW_NEGATIVE_REGRESSIONS=FAIL")

print("SLICE2_PREVIEW_SOURCE_BOUNDARY=PASS")
print("VDR_2_8_3_NATIVE_HEADER_COMPILE=NOT_YET_TESTED")
print("PUBLIC_TIMER_CREATE_ENABLED=NO")
