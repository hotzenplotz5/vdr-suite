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
    "StableTypeWindowCache" in plugin_command
    and "BuildStableWindowSnapshot" in plugin_command
    and "window.fromTime == request.FromTime()" in plugin_command
    and "window.untilTime == request.UntilTime()" in plugin_command,
    "ETYPES offsets must address a retained immutable backend window snapshot",
)
require(
    "LOCK_CHANNELS_READ;" in plugin_command
    and "LOCK_SCHEDULES_READ;" in plugin_command
    and "GetEventById(identity.eventId)" in plugin_command
    and "adapter.ResolveMediaType(*event)" in plugin_command,
    "ETYPES must re-resolve and classify the real locked VDR schedule event",
)
for forbidden in (
    "new cEvent(",
    "cSchedule schedule_",
    "SetTitle(source.Title())",
    "SuiteBridgeTypeSnapshotEvent",
):
    require(
        forbidden not in plugin_command,
        "ETYPES must not manufacture detached cEvent copies",
    )
require(
    "SuiteBridgeEpgTypeSnapshotPayload bounded(candidate)" in plugin_command
    and "if (!bounded.Complete())" in plugin_command
    and "MaximumRetainedWindows = 4" in plugin_command
    and "MaximumSnapshotEvents = 100000" in plugin_command,
    "ETYPES pagination must stay within memory, event-count and SVDRP bounds",
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
    and "endTime <= requestedFrom" in transport
    and "startTime >= requestedUntil" in transport
    and "page.payloadValid = parsePayload" in transport,
    "agent transport must validate cursor and requested-window isolation",
)
require(
    "applyEpgTypeSnapshot(" in runtime_header
    and 'providerId = "tvscraper-media-type"' in runtime
    and 'sourceKind = "scraper-media-type"' in runtime
    and 'state = "active"' in runtime
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
apply_failure = worker.find("if (!applied)")
apply_return = worker.find("return false;", apply_failure)
offset_advance = worker.find("context.epgTypeSnapshotOffset = page.nextOffset")
require(
    0 <= apply_failure < apply_return < offset_advance,
    "failed ETYPES persistence must not advance the backend cursor",
)
require(
    "tryHandleGet" not in runtime,
    "ETYPES materialization must not add a public HTTP/provider route",
)

print("EPG type snapshot architecture contracts ok")
