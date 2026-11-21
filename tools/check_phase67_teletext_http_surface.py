#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "api/rest/include/TeletextApiRuntime.h"
SOURCE = ROOT / "api/rest/src/TeletextApiRuntime.cpp"
TEST = ROOT / "api/rest/tests/test_teletext_api_runtime.cpp"
API_ROUTER = ROOT / "api/rest/include/ApiRouter.h"
DAEMON_RUNTIME = ROOT / "core/daemon/src/DaemonTeletextRuntime.cpp"

errors: list[str] = []

for path in (HEADER, SOURCE, TEST, API_ROUTER, DAEMON_RUNTIME):
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext HTTP-surface file: {path.relative_to(ROOT)}")

header = HEADER.read_text(encoding="utf-8") if HEADER.is_file() else ""
source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
test = TEST.read_text(encoding="utf-8") if TEST.is_file() else ""
router = API_ROUTER.read_text(encoding="utf-8") if API_ROUTER.is_file() else ""
daemon = DAEMON_RUNTIME.read_text(encoding="utf-8") if DAEMON_RUNTIME.is_file() else ""

for fragment in (
    "class TeletextApiRuntime",
    "bool configure(TeletextControlPlaneReadService& readService);",
    "bool tryHandleGet(",
    "TeletextControlPlaneReadService* readService_ = nullptr;",
):
    if fragment not in header:
        errors.append(f"TeletextApiRuntime header missing fragment: {fragment}")

for fragment in (
    '"/api/vdr/broadcast/teletext/service"',
    '"/api/vdr/broadcast/teletext/page"',
    "MaximumBackendIdBytes = 128U",
    "MaximumChannelIdBytes = 63U",
    "pageValue < 100U",
    "0xfffeU",
    'response.headers["Cache-Control"] = "no-store";',
    '"application/json; charset=utf-8"',
    "readService_->discoverService",
    "readService_->readPage",
    "appendServiceRef",
    "snapshot.cells",
):
    if fragment not in source:
        errors.append(f"TeletextApiRuntime source missing fragment: {fragment}")

for forbidden in (
    "TTXC 1",
    "TTXP 1",
    "/var/cache/vdr/vtx",
    "OsdTeletext::",
):
    if forbidden in source or forbidden in header:
        errors.append(f"HTTP surface must not own provider/wire detail: {forbidden}")

for fragment in (
    '#include "TeletextApiRuntime.h"',
    "TeletextApiRuntime::instance().tryHandleGet(",
):
    if fragment not in router:
        errors.append(f"ApiRouter missing Teletext HTTP exposure fragment: {fragment}")

for fragment in (
    '#include "TeletextApiRuntime.h"',
    "TeletextApiRuntime::instance().configure(*readService)",
    "TeletextApiRuntime::instance().reset();",
):
    if fragment not in daemon:
        errors.append(f"Daemon Teletext runtime missing API ownership fragment: {fragment}")

for fragment in (
    "Börse und Nachrichten",
    "teletext_service_unavailable",
    "subpage=65535",
    "C%20bad",
    "teletext_runtime_unavailable",
):
    if fragment not in test:
        errors.append(f"Teletext HTTP regression test missing fragment: {fragment}")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 67 Teletext HTTP surface contract ok")
