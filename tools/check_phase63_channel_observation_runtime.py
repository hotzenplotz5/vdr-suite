#!/usr/bin/env python3
"""Architecture guard for the Phase-63 Channel observation runtime."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BASE_COMMIT = "2567407e6e1c6d098804f887875e7f3cbf9cba60"
BASE_TREE = "c4afb5d9c413401476e642c8799178a8e7676d33"

files = {
    "contract": ROOT / "docs/development/phase-63-channel-observation-ingestion.md",
    "runtime": ROOT / "docs/development/phase-63-channel-observation-runtime.md",
    "channel_header": ROOT / "core/agent/include/BackendAgentChannelObservation.h",
    "channel_source": ROOT / "core/agent/src/BackendAgentChannelObservation.cpp",
    "json_source": ROOT / "core/agent/src/BackendAgentChannelObservationJson.cpp",
    "client": ROOT / "core/agent/src/BackendAgentClient.cpp",
    "lifecycle": ROOT / "core/agent/src/BackendAgentLifecycle.cpp",
    "repository": ROOT / "core/agent/src/BackendAgentRepository.cpp",
    "http": ROOT / "core/agent/src/BackendAgentHttpServer.cpp",
    "schema": ROOT / "database/schema/vdr-suite.sql",
    "admin": ROOT / "apps/tools/backend_agent_admin.cpp",
    "config": ROOT / "packaging/systemd/backend-agent.conf",
    "make": ROOT / "mk/agent-sources.mk",
}
failures: list[str] = []
texts: dict[str, str] = {}
for name, path in files.items():
    if not path.is_file():
        failures.append(f"missing required file: {path.relative_to(ROOT)}")
        texts[name] = ""
    else:
        texts[name] = path.read_text(encoding="utf-8")


def require(name: str, markers: list[str]) -> None:
    text = texts[name]
    for marker in markers:
        if marker not in text:
            failures.append(f"{files[name].relative_to(ROOT)} misses marker: {marker}")


require("runtime", [
    BASE_COMMIT, BASE_TREE, "channels-conf", "CHANNELS_CONF_PATH",
    "<IDENTITY_PATH>.channels.pending.json", "backend_agent_channel_facts",
    "VdrChannelCacheRepository", "manual SQLite inspection",
    "Real yaVDR acceptance gate",
])
require("channel_source", [
    "MaximumChannelsConfBytes", "MaximumChannels", "channelIdFor",
    "channels_conf_duplicate_channel_id", "backendAgentCanonicalChannelPayload",
])
require("json_source", [
    "MaximumBodyBytes", "objectValue.emplace", "exactKeys", "removedChannelIds",
    "invalid_channel_observation_json",
])
require("client", [
    '"channels-conf"', '"channels"', "channelsConfPath",
    "pendingChannelObservationPath", "publishChannelObservation",
    '"/api/agent/v1/observations/channels"', "MaximumObservationRequestBytes",
])
require("lifecycle", [
    'request.observationDomain == "channels"', "backendAgentValidChannelFact",
    "backendAgentCanonicalChannelPayload", "invalid_removed_channel_id",
])
require("repository", [
    "backend_agent_channel_facts", "channelFactsForBackend",
    "unknown_channel_removal", "BEGIN IMMEDIATE",
])
require("http", [
    '"/api/agent/v1/observations/channels"', "handleChannelObservation",
    "MaximumChannelObservationBodyBytes",
])
require("schema", ["backend_agent_channel_facts", "PRIMARY KEY (backend_id, channel_id)"])
require("admin", ["channelObservation", "factCount", '"channels"'])
require("config", ["CHANNELS_CONF_PATH=/var/lib/vdr/channels.conf"])
require("make", [
    "AGENT_CHANNEL_DOMAIN_SRC", "BackendAgentChannelObservation.cpp",
    "AGENT_CHANNEL_JSON_SRC", "BackendAgentChannelObservationJson.cpp",
])

for name in ("client", "http"):
    text = texts[name]
    if "sqlite3_" in text or "INSERT INTO " in text or "SELECT " in text:
        failures.append(f"{files[name].relative_to(ROOT)} crossed repository-owned SQLite boundary")

combined = "\n".join(texts[name] for name in ("channel_source", "json_source", "client", "lifecycle", "http"))
for forbidden in [
    "VdrTimerAction", "CommandInbox", "SvdrpChannelMove", "updateBackendOnline",
    "providerUrl", "provider_url",
]:
    if forbidden in combined:
        failures.append(f"Channel runtime crossed excluded boundary: {forbidden}")

if "vdr_channel_cache" in texts["repository"]:
    failures.append("Agent-owned Channel repository must not write the direct-adapter cache")

if failures:
    print("Phase-63 Channel observation runtime check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 Channel observation runtime check passed")
print(f"Merged Channel contract base: {BASE_COMMIT}")
print(f"Merged Channel contract tree: {BASE_TREE}")
print("Explicit source: channels-conf")
print("Authority: Agent-owned facts only")
