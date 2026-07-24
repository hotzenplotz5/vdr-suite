#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


plugin_svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
plugin_command = read(
    "vdr-plugin-suite-bridge/suitebridge_epg_type_snapshot_command.cpp"
)
plugin_contract = read(
    "vdr-plugin-suite-bridge/suitebridge_epg_type_snapshot_contract.cpp"
)
plugin_type_adapter = read(
    "vdr-plugin-suite-bridge/suitebridge_tvscraper_type_adapter.cpp"
)
transport_header = read("core/agent/include/SuiteBridgeSvdrpTransport.h")
transport = read(
    "core/agent/src/SuiteBridgeSvdrpEpgTypeSnapshotTransport.cpp"
)
runtime_header = read("api/rest/include/GenreBrowserApiRuntime.h")
runtime = read("api/rest/src/GenreBrowserApiRuntimeEpgTypeSnapshot.cpp")
worker = read("core/daemon/src/DaemonRuntimeEpgCache.cpp")
context = read("core/daemon/include/BackendRuntimeContext.h")

require(
    '"ETYPES <from-epoch> <until-epoch> <offset> <limit>\\n"'
    in plugin_svdrp
    and "HandleTypeSnapshot(Command, Option)" in plugin_svdrp,
    "SuiteBridge must expose the bounded read-only ETYPES command",
)
require(
    "LOCK_CHANNELS_READ;" in plugin_command
    and "LOCK_SCHEDULES_READ;" in plugin_command
    and "schedule->Events()->First()" in plugin_command
    and "adapter.ResolveMediaType(event)" in plugin_command,
    "ETYPES must scan real locked VDR events and resolve their TVScraper type",
)
require(
    "captured.push_back" in plugin_command
    and plugin_command.find("LOCK_SCHEDULES_READ;")
    < plugin_command.find("adapter.ResolveMediaType(event)"),
    "TVScraper calls must run on detached event snapshots after VDR locks are released",
)
require(
    "cGetScraperVideo request(&event, nullptr)" in plugin_type_adapter
    and "request.m_scraperVideo->getVideoType()" in plugin_type_adapter,
    "fast type lookup must mirror Live's direct GetScraperVideo/getVideoType path",
)
require(
    "until - from > 72ULL * 60ULL * 60ULL" in plugin_contract
    and "limit == 0 || limit > 64" in plugin_contract
    and "kCapacity = 7680" in read(
        "vdr-plugin-suite-bridge/suitebridge_epg_type_snapshot_contract.h"
    ),
    "ETYPES request and payload must remain hard bounded",
)
require(
    "public ::ISuiteBridgeEpgTypeSnapshotTransport" in transport_header
    and "requestEpgTypeSnapshot(" in transport_header,
    "agent transport must expose a typed EPG snapshot boundary",
)
require(
    '"PLUG suitebridge ETYPES "' in transport
    and "nextOffset != requestedOffset + scanned" in transport
    and "page.payloadValid = parsePayload" in transport,
    "agent transport must validate bounded ETYPES pages",
)
require(
    "applyEpgTypeSnapshot(" in runtime_header
    and "providerId = \"tvscraper-media-type\"" in runtime
    and "sourceKind = \"scraper-media-type\"" in runtime
    and "state = \"active\"" in runtime
    and "reconcileEpgBrowseClassification" in runtime,
    "daemon runtime must persist snapshot types as authoritative TVScraper evidence",
)
require(
    "EpgTypeSnapshotPageSize = 64" in worker
    and "InitialEpgTypeSnapshotPages = 32" in worker
    and "PeriodicEpgTypeSnapshotPages = 8" in worker
    and "processEpgTypeSnapshotPages" in worker,
    "EPG worker must drain bounded startup and continuation pages",
)
require(
    "epgTypeSnapshotOffset" in context
    and "epgTypeSnapshotComplete" in context
    and "initializeSnapshot" in worker,
    "backend-scoped snapshot progress must survive dirty EPG refreshes",
)
require(
    worker.find("if (initializeSnapshot)")
    < worker.find("processEpgTypeSnapshotPages(", worker.find("if (initializeSnapshot)")),
    "dirty refreshes must continue an incomplete snapshot instead of resetting it",
)
require(
    "tryHandleGet" not in runtime,
    "ETYPES materialization must not add a public HTTP/provider route",
)

print("EPG type snapshot architecture contracts ok")
