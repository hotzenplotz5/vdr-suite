#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

HEADER = ROOT / "core/agent/include/SuiteBridgeSvdrpTransport.h"
SOURCE = ROOT / "core/agent/src/SuiteBridgeSvdrpTransport.cpp"
TEST = ROOT / "core/agent/tests/test_suite_bridge_svdrp_transport.cpp"
LIVE_TEST = ROOT / "core/agent/tests/test_suite_bridge_svdrp_transport_live.cpp"

REQUIRED = [
    HEADER,
    SOURCE,
    TEST,
    LIVE_TEST,
]

FORBIDDEN_SOURCE_TOKENS = [
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
    "SvdrpChannelMoveExecutor",
    "vdr-plugin-suite-bridge/",
]

REQUIRED_HEADER_FRAGMENTS = [
    "struct SuiteBridgeSvdrpTransportConfig",
    "class SuiteBridgeSvdrpTransport final : public ISuiteBridgeLocalTransport",
    "MaximumGreetingBytes = 1024",
    "MaximumReplyBytes = 8192",
    "MaximumReplyLines = 64",
    "SuiteBridgeCommandReply execute(",
]

REQUIRED_SOURCE_FRAGMENTS = [
    'return "PLUG suitebridge CAPS 1\\r\\n";',
    'return "PLUG suitebridge SNAP\\r\\n";',
    "AI_NUMERICHOST | AI_NUMERICSERV",
    "O_NONBLOCK",
    "FD_CLOEXEC",
    "poll(&descriptor, 1, timeout)",
    "SO_ERROR",
    "MSG_NOSIGNAL",
    "greeting.code != 220",
    "separator != ' ' && separator != '-'",
    "SVDRP reply exceeds bounded size",
    "inconsistent SVDRP multiline reply code",
]

REQUIRED_TEST_FRAGMENTS = [
    'server.request() == "PLUG suitebridge CAPS 1\\r\\n"',
    'server.request() == "PLUG suitebridge SNAP\\r\\n"',
    "testMultilineReply();",
    "testReplyTimeout();",
    "testGreetingTimeout();",
    "testOversizedReply();",
    "testConnectionFailure();",
]

errors: list[str] = []

for path in REQUIRED:
    if not path.is_file():
        errors.append(
            f"missing required SB.10b file: {path.relative_to(ROOT)}"
        )

header_text = HEADER.read_text(encoding="utf-8") if HEADER.is_file() else ""
source_text = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
test_text = TEST.read_text(encoding="utf-8") if TEST.is_file() else ""

for token in FORBIDDEN_SOURCE_TOKENS:
    if token in header_text or token in source_text:
        errors.append(
            f"forbidden SB.10b transport token {token!r}"
        )

for fragment in REQUIRED_HEADER_FRAGMENTS:
    if fragment not in header_text:
        errors.append(
            f"missing typed SB.10b header contract: {fragment}"
        )

for fragment in REQUIRED_SOURCE_FRAGMENTS:
    if fragment not in source_text:
        errors.append(
            f"missing bounded SB.10b source contract: {fragment}"
        )

for fragment in REQUIRED_TEST_FRAGMENTS:
    if fragment not in test_text:
        errors.append(
            f"missing SB.10b transport test contract: {fragment}"
        )

if "std::string command" in header_text:
    errors.append("SVDRP transport must not accept arbitrary command text")

if "const std::string& command" in header_text:
    errors.append("SVDRP transport must not expose a free-form command")

expected_requests = {
    '"PLUG suitebridge CAPS 1\\r\\n"',
    '"PLUG suitebridge SNAP\\r\\n"',
}

observed_requests = {
    line.strip().removeprefix("return ").removesuffix(";")
    for line in source_text.splitlines()
    if "PLUG suitebridge " in line
}

if observed_requests != expected_requests:
    errors.append(
        "SB.10b source must contain only the two fixed Suite Bridge requests"
    )

if errors:
    for error in errors:
        print(f"ERROR: {error}")
    sys.exit(1)

print("suite bridge SVDRP transport boundary contract ok")
