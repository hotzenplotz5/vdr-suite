#!/usr/bin/env python3
"""Static ownership guard: public EPG EventID is not a synthetic cache key.

No DB or daemon is touched. Focused C++ tests verify value behavior separately.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def content(path):
    return (ROOT / path).read_text(encoding="utf-8")


def require(test, reason):
    if not test:
        raise SystemExit("EPG_TIMER_SOURCE_ID_GUARD=FAIL:" + reason)


repository = content("core/vdr/src/EpgEventRepository.cpp")
header = content("core/vdr/include/EpgEventRepository.h")
cache_service = content("core/vdr/src/EpgCacheService.cpp")
runtime = content("core/daemon/src/DaemonRuntimeInitialization.cpp")
intent = content("core/timers/include/EpgTimerIntentDraft.h")
selector = content("core/timers/include/EpgTimerSelectionGuard.h")
epg_test = content("core/vdr/tests/test_epg_event_repository.cpp")

require('return event.channelId + ":" + event.startTime + ":" + event.title;' in repository,
        "CACHE_FALLBACK_INVARIANT_MOVED_REAUDIT_REQUIRED")
require("epg_event_id_source_evidence" in repository and
        "AFTER DELETE ON epg_events" in repository,
        "SOURCE_PROVENANCE_SCHEMA_MISSING")
require('source_id_present=excluded.source_id_present' in repository and
        'event.id.empty() ? 0 : 1' in repository,
        "SOURCE_PROVENANCE_UPSERT_MISSING")
require("hasSourceEventIdForBackend(" in repository and
        "hasSourceEventIdForBackend(" in header and
        "INNER JOIN epg_events" in repository,
        "EXACT_ROW_PROVENANCE_READ_MISSING")
require("readRepository_->hasSourceEventIdForBackend(" in cache_service,
        "READ_REPOSITORY_OWNERSHIP_LOST")
start = runtime.index("registerEpgWindowLookup(")
end = runtime.index("registerChannelCollectionLookup(", start)
window = runtime[start:end]
require('service->hasSourceEventIdForBackend(' in window and
        'item.eventId = event.id;' in window and
        window.index("service->hasSourceEventIdForBackend(") <
        window.index("item.eventId = event.id;"),
        "PUBLIC_ID_MUST_BE_SOURCE_PROVEN")
require("if (event.id.size() <= 256U)" not in window,
        "UNVERIFIED_ID_LEAK_REGRESSION")
require("timerIntentValidSpec(spec)" in intent and
        "observed.nativeEventId" in intent and
        "observed.exactNativeIdentity" in selector,
        "TIMER_INTENT_AUTHORITY_GAP")
require("test_event_id_source_evidence_is_durable_and_fail_closed" in epg_test and
        "test_unchanged_upsert_does_not_rewrite_row" in epg_test,
        "SOURCE_REGRESSION_NOT_REGISTERED")
print("EPG_TIMER_SOURCE_ID_GUARD=PASS")
