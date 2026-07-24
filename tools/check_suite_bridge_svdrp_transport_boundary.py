#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

HEADER = ROOT / "core/agent/include/SuiteBridgeSvdrpTransport.h"
SOURCE = ROOT / "core/agent/src/SuiteBridgeSvdrpTransport.cpp"
TYPE_SNAPSHOT_SOURCE = (
    ROOT / "core/agent/src/SuiteBridgeSvdrpEpgTypeSnapshotTransport.cpp"
)
METADATA_SOURCE = ROOT / "core/agent/src/SuiteBridgeSvdrpMetadataTransport.cpp"
TEST = ROOT / "core/agent/tests/test_suite_bridge_svdrp_transport.cpp"
TYPE_SNAPSHOT_TEST = (
    ROOT / "core/agent/tests/test_suite_bridge_svdrp_epg_type_snapshot_transport.cpp"
)
METADATA_TEST = ROOT / "core/agent/tests/test_suite_bridge_svdrp_metadata_transport.cpp"
LIVE_TEST = ROOT / "core/agent/tests/test_suite_bridge_svdrp_transport_live.cpp"

REQUIRED = [
    HEADER,
    SOURCE,
    TYPE_SNAPSHOT_SOURCE,
    METADATA_SOURCE,
    TEST,
    TYPE_SNAPSHOT_TEST,
    METADATA_TEST,
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
    "class SuiteBridgeSvdrpTransport final :",
    "public ISuiteBridgeLocalTransport",
    "public ::ISuiteBridgeArtworkTransport",
    "public ::ISuiteBridgeEpgTypeSnapshotTransport",
    "public ::ISuiteBridgeMetadataTransport",
    "MaximumGreetingBytes = 1024",
    "MaximumReplyBytes = 8192",
    "MaximumReplyLines = 64",
    "SuiteBridgeCommandReply execute(",
    "requestArtwork(",
    "requestEpgTypeSnapshot(",
    "requestMetadata(",
]

REQUIRED_SOURCE_FRAGMENTS = [
    'return "PLUG suitebridge CAPS 1\\r\\n";',
    'return "PLUG suitebridge SNAP\\r\\n";',
    '"PLUG suitebridge ARTW " + channelId + " " + eventId + "\\r\\n"',
    "safeToken(channelId)",
    "safeToken(eventId)",
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

REQUIRED_TYPE_SNAPSHOT_SOURCE_FRAGMENTS = [
    '"PLUG suitebridge ETYPES " + std::to_string(fromTime)',
    "limit == 0 || limit > 64",
    "nextOffset != requestedOffset + scanned",
    'fields[4] != "S" && fields[4] != "M"',
    "page.payloadValid = parsePayload",
]

REQUIRED_METADATA_SOURCE_FRAGMENTS = [
    '"PLUG suitebridge META " + channelId + " " + eventId + "\\r\\n"',
    "safeMetadataToken(channelId)",
    "safeMetadataToken(eventId)",
    "reply.transportSucceeded() && reply.replyCode == 250",
    "metadataReply.replyCode = reply.replyCode",
    "metadataReply.payload = reply.payload",
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

REQUIRED_TYPE_SNAPSHOT_TEST_FRAGMENTS = [
    '"PLUG suitebridge ETYPES 100 300 0 64\\r\\n"',
    "page.payloadValid",
    "page.items[0].mediaType == EpgScraperMediaType::Series",
    "page.items[1].mediaType == EpgScraperMediaType::Movie",
    "!page.payloadValid",
]

REQUIRED_METADATA_TEST_FRAGMENTS = [
    '"PLUG suitebridge META S19.2E-1-1011-11100 12345\\r\\n"',
    "reply.transportSucceeded",
    "reply.replyCode == 250",
    'Server server("451 Metadata payload exceeds contract capacity\\r\\n")',
    "!failed.transportSucceeded",
    "failed.replyCode == 451",
    "bad channel",
    "bad event",
]

errors: list[str] = []

for path in REQUIRED:
    if not path.is_file():
        errors.append(
            f"missing required SB.10b file: {path.relative_to(ROOT)}"
        )

header_text = HEADER.read_text(encoding="utf-8") if HEADER.is_file() else ""
source_text = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
type_snapshot_source_text = (
    TYPE_SNAPSHOT_SOURCE.read_text(encoding="utf-8")
    if TYPE_SNAPSHOT_SOURCE.is_file()
    else ""
)
metadata_source_text = (
    METADATA_SOURCE.read_text(encoding="utf-8")
    if METADATA_SOURCE.is_file()
    else ""
)
test_text = TEST.read_text(encoding="utf-8") if TEST.is_file() else ""
type_snapshot_test_text = (
    TYPE_SNAPSHOT_TEST.read_text(encoding="utf-8")
    if TYPE_SNAPSHOT_TEST.is_file()
    else ""
)
metadata_test_text = (
    METADATA_TEST.read_text(encoding="utf-8")
    if METADATA_TEST.is_file()
    else ""
)

for token in FORBIDDEN_SOURCE_TOKENS:
    if (
        token in header_text
        or token in source_text
        or token in type_snapshot_source_text
        or token in metadata_source_text
    ):
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

for fragment in REQUIRED_TYPE_SNAPSHOT_SOURCE_FRAGMENTS:
    if fragment not in type_snapshot_source_text:
        errors.append(
            f"missing bounded EPG type snapshot transport contract: {fragment}"
        )

for fragment in REQUIRED_METADATA_SOURCE_FRAGMENTS:
    if fragment not in metadata_source_text:
        errors.append(
            f"missing bounded EPG metadata transport contract: {fragment}"
        )

for fragment in REQUIRED_TEST_FRAGMENTS:
    if fragment not in test_text:
        errors.append(
            f"missing SB.10b transport test contract: {fragment}"
        )

for fragment in REQUIRED_TYPE_SNAPSHOT_TEST_FRAGMENTS:
    if fragment not in type_snapshot_test_text:
        errors.append(
            f"missing EPG type snapshot transport test contract: {fragment}"
        )

for fragment in REQUIRED_METADATA_TEST_FRAGMENTS:
    if fragment not in metadata_test_text:
        errors.append(
            f"missing EPG metadata transport test contract: {fragment}"
        )

if "std::string command" in header_text:
    errors.append("SVDRP transport must not accept arbitrary command text")

if "const std::string& command" in header_text:
    errors.append("SVDRP transport must not expose a free-form command")

expected_fixed_requests = {
    '"PLUG suitebridge CAPS 1\\r\\n"',
    '"PLUG suitebridge SNAP\\r\\n"',
}

observed_fixed_requests = {
    line.strip().removeprefix("return ").removesuffix(";")
    for line in source_text.splitlines()
    if "PLUG suitebridge " in line and "return " in line
}

if observed_fixed_requests != expected_fixed_requests:
    errors.append(
        "SB.10b source must retain exactly the two fixed local Suite Bridge requests"
    )

if source_text.count('"PLUG suitebridge ARTW "') != 1:
    errors.append(
        "SB.10b source must contain exactly one bounded artwork request prefix"
    )

if type_snapshot_source_text.count('"PLUG suitebridge ETYPES "') != 1:
    errors.append(
        "EPG type snapshot transport must contain exactly one bounded ETYPES request prefix"
    )

if metadata_source_text.count('"PLUG suitebridge META "') != 1:
    errors.append(
        "EPG metadata transport must contain exactly one bounded META request prefix"
    )

if errors:
    for error in errors:
        print(f"ERROR: {error}")
    sys.exit(1)

print("suite bridge SVDRP transport boundary contract ok")
