#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED = [
    ROOT / "core/agent/include/ISuiteBridgeLocalTransport.h",
    ROOT / "core/agent/include/SuiteBridgeHandshake.h",
    ROOT / "core/agent/include/SuiteBridgeLocalContractParser.h",
    ROOT / "core/agent/include/SuiteBridgeHandshakeService.h",
    ROOT / "core/agent/src/SuiteBridgeHandshake.cpp",
    ROOT / "core/agent/src/SuiteBridgeLocalContractParser.cpp",
    ROOT / "core/agent/src/SuiteBridgeHandshakeService.cpp",
    ROOT / "core/agent/tests/test_suite_bridge_handshake.cpp",
]

FORBIDDEN_SOURCE_TOKENS = [
    "popen(",
    "system(",
    "fork(",
    "execv",
    "socket(",
    "connect(",
    "std::thread",
    "std::mutex",
    "std::filesystem",
    "std::fstream",
    "sqlite3",
    "Database.h",
    "DaemonRuntime.h",
    "RestfulApi",
    "SvdrpChannelMoveExecutor",
    "vdr-plugin-suite-bridge/",
]

REQUIRED_TRANSPORT_FRAGMENTS = [
    "enum class SuiteBridgeLocalCommand",
    "DiscoverSchema1",
    "Snapshot",
    "virtual SuiteBridgeCommandReply execute(",
]

REQUIRED_HANDSHAKE_FRAGMENTS = [
    "transport_.execute(SuiteBridgeLocalCommand::DiscoverSchema1)",
    "transport_.execute(SuiteBridgeLocalCommand::Snapshot)",
    "discoveryReply.replyCode != 900",
    "snapshotReply.replyCode != 900",
    "discovery.discoverySchema != 1",
    "discovery.capabilitySchema != 1",
    "discovery.snapshotSchema != 2",
    "discovery.localContractSchema != 2",
    "result.mutationsEnabled = false",
]

REQUIRED_PARSER_FRAGMENTS = [
    "MaximumPayloadBytes = 4096",
    "MaximumCapabilities = 64",
    "duplicate capability id",
    "invalid counter epoch",
    "snapshot total does not match counters",
]

errors: list[str] = []

for path in REQUIRED:
    if not path.is_file():
        errors.append(f"missing required SB.10a file: {path.relative_to(ROOT)}")

source_paths = [
    path
    for path in REQUIRED
    if path.is_file() and "/tests/" not in path.as_posix()
]

for path in source_paths:
    text = path.read_text(encoding="utf-8")

    for token in FORBIDDEN_SOURCE_TOKENS:
        if token in text:
            errors.append(
                f"forbidden Agent-boundary token {token!r} in "
                f"{path.relative_to(ROOT)}"
            )

transport_path = ROOT / "core/agent/include/ISuiteBridgeLocalTransport.h"
if transport_path.is_file():
    transport_text = transport_path.read_text(encoding="utf-8")

    for fragment in REQUIRED_TRANSPORT_FRAGMENTS:
        if fragment not in transport_text:
            errors.append(f"missing typed transport contract: {fragment}")

    if "std::string command" in transport_text:
        errors.append("local bridge transport must not accept arbitrary commands")

handshake_path = ROOT / "core/agent/src/SuiteBridgeHandshakeService.cpp"
if handshake_path.is_file():
    handshake_text = handshake_path.read_text(encoding="utf-8")

    for fragment in REQUIRED_HANDSHAKE_FRAGMENTS:
        if fragment not in handshake_text:
            errors.append(f"missing fail-closed handshake contract: {fragment}")

    discovery_position = handshake_text.find(
        "transport_.execute(SuiteBridgeLocalCommand::DiscoverSchema1)"
    )
    snapshot_position = handshake_text.find(
        "transport_.execute(SuiteBridgeLocalCommand::Snapshot)"
    )

    if discovery_position < 0 or snapshot_position < 0:
        pass
    elif discovery_position >= snapshot_position:
        errors.append("SNAP must never be requested before compatible CAPS discovery")

parser_header = ROOT / "core/agent/include/SuiteBridgeLocalContractParser.h"
parser_source = ROOT / "core/agent/src/SuiteBridgeLocalContractParser.cpp"
parser_text = ""

if parser_header.is_file():
    parser_text += parser_header.read_text(encoding="utf-8")

if parser_source.is_file():
    parser_text += parser_source.read_text(encoding="utf-8")

for fragment in REQUIRED_PARSER_FRAGMENTS:
    if fragment not in parser_text:
        errors.append(f"missing bounded parser contract: {fragment}")

plugin_directory = ROOT / "vdr-plugin-suite-bridge"
for forbidden_name in (
    "suitebridge_agent.cpp",
    "suitebridge_transport.cpp",
    "suitebridge_handshake.cpp",
):
    if (plugin_directory / forbidden_name).exists():
        errors.append(
            "Backend Agent implementation must not be placed in the plugin: "
            + forbidden_name
        )

if errors:
    for error in errors:
        print(f"ERROR: {error}")
    sys.exit(1)

print("suite bridge Agent boundary contract ok")
