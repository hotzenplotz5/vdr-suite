#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

INTERFACE = ROOT / "core/vdr/include/ISuiteBridgeTeletextTransport.h"
HEADER = ROOT / "core/agent/include/SuiteBridgeSvdrpTransport.h"
SOURCE = ROOT / "core/agent/src/SuiteBridgeSvdrpTeletextTransport.cpp"
TEST = ROOT / "core/agent/tests/test_suite_bridge_svdrp_teletext_transport.cpp"
SOURCES = ROOT / "mk/agent-sources.mk"

required = (INTERFACE, HEADER, SOURCE, TEST, SOURCES)
errors: list[str] = []

for path in required:
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext agent file: {path.relative_to(ROOT)}")

interface = INTERFACE.read_text(encoding="utf-8") if INTERFACE.is_file() else ""
header = HEADER.read_text(encoding="utf-8") if HEADER.is_file() else ""
source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
test = TEST.read_text(encoding="utf-8") if TEST.is_file() else ""
sources = SOURCES.read_text(encoding="utf-8") if SOURCES.is_file() else ""

for fragment in (
    "struct SuiteBridgeTeletextCommandReply",
    "struct SuiteBridgeTeletextPageRequest",
    "class ISuiteBridgeTeletextTransport",
    "discoverTeletext()",
    "requestTeletextPage(",
):
    if fragment not in interface:
        errors.append(f"missing typed Teletext transport interface: {fragment}")

for fragment in (
    '#include "ISuiteBridgeTeletextTransport.h"',
    "public ::ISuiteBridgeTeletextTransport",
    "discoverTeletext() override",
    "requestTeletextPage(",
):
    if fragment not in header:
        errors.append(f"missing SuiteBridge transport Teletext wiring: {fragment}")

for fragment in (
    '"PLUG suitebridge TTXC 1\\r\\n"',
    '"PLUG suitebridge TTXP 1 "',
    "safeTeletextChannelId(request.channelId)",
    "request.pageNumber < 100 || request.pageNumber > 899",
    "request.subpageCode == 0xffffU",
    "reply.transportSucceeded() && reply.replyCode == 250",
):
    if fragment not in source:
        errors.append(f"missing bounded Teletext SVDRP transport contract: {fragment}")

for fragment in (
    '"PLUG suitebridge TTXC 1\\r\\n"',
    '"PLUG suitebridge TTXP 1 C-1-1051-10301 100 auto\\r\\n"',
    '"PLUG suitebridge TTXP 1 C-1-1051-10301 777 42\\r\\n"',
    'request.channelId = "bad channel"',
    "request.pageNumber = 99",
    "request.pageNumber = 900",
    "request.subpageCode = 0xffffU",
    "failed.replyCode == 550",
):
    if fragment not in test:
        errors.append(f"missing Teletext transport regression coverage: {fragment}")

if "core/agent/src/SuiteBridgeSvdrpTeletextTransport.cpp" not in sources:
    errors.append("Teletext SVDRP transport is not part of AGENT_SVDRP_TRANSPORT_SRC")

for token in (
    "popen(",
    "system(",
    "fork(",
    "execv",
    "svdrpsend",
    "std::thread",
    "std::mutex",
    "std::filesystem",
    "std::fstream",
    "sqlite3",
    "Database.h",
    "DaemonRuntime.h",
    "BasicHttpClient",
    "RestfulApi",
    "vdr-plugin-suite-bridge/",
):
    if token in source:
        errors.append(f"forbidden Teletext agent transport dependency: {token}")

if source.count('"PLUG suitebridge TTXC 1\\r\\n"') != 1:
    errors.append("Teletext transport must contain exactly one fixed TTXC request")

if source.count('"PLUG suitebridge TTXP 1 "') != 1:
    errors.append("Teletext transport must contain exactly one bounded TTXP request prefix")

if errors:
    for error in errors:
        print(f"ERROR: {error}")
    sys.exit(1)

print("Phase 67 Teletext agent transport boundary contract ok")
