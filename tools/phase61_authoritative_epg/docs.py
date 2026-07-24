from .common import append_once

# ---------------------------------------------------------------------------
# Documentation and handoff.
# ---------------------------------------------------------------------------

append_once(
    "docs/architecture/metadata-genre-browser.md",
    "## Phase 61 EPG Cache Reconciliation",
    r'''
## Phase 61 EPG Cache Reconciliation

The real-system diagnosis on 2026-07-24 proved that RESTfulAPI may replace
backend-native event IDs for the same schedule occurrence while the warm cache
still retains the previous rows. The cache now reconciles only an explicitly
bounded, authoritative refresh window:

- native event IDs remain backend-scoped cache identities;
- title, subtitle and schedule similarity are never promoted to canonical
  identity;
- channels that hit `channelEventLimit` are treated as potentially truncated
  and are not destructively reconciled;
- missing native IDs are removed only inside the proven backend/channel/time
  scope;
- dependent artwork and scraper-cache rows are removed, while Genre target
  evidence is retired/staled instead of silently rebound;
- current native IDs are enriched normally through the existing asynchronous
  SuiteBridge path;
- metadata GETs for retired cache IDs return `stale-event` and never enqueue a
  provider request;
- pending frontend metadata responses are retried with a bounded backoff and
  are not cached permanently.

The EPG timeline and PR #99 LiveRemote/overlay route ownership remain unchanged.
'''
)

append_once(
    "docs/NEW-CHAT-HANDOFF.md",
    "## Phase 61 EPG Identity Diagnosis and Fix",
    r'''
## Phase 61 EPG Identity Diagnosis and Fix

Real-system evidence from 2026-07-24 confirmed stale backend-native EPG IDs in
`epg_events` after RESTfulAPI had already published replacement IDs. Examples
included `38843 -> 39566`, `38844 -> 39567` and `38845 -> 39568` on the default
backend. The implementation direction is authoritative window reconciliation,
not title/time deduplication.

The current patch:

- uses an explicit epoch-based warmup window;
- reconciles only channels proven complete below the per-channel cap;
- removes stale cache rows and dependent scraper/artwork rows atomically;
- retires old Genre bindings without copying identity-bound public JSON;
- rejects metadata materialization for IDs no longer present in `epg_events`;
- retries frontend `pending` responses without permanently caching them;
- defines the cache-refresh execution gate out of line for one process-wide
  instance.

Do not restore the former +/-5-second title matcher or copy metadata JSON from
an old native event ID to a new one.
'''
)

append_once(
    "docs/CURRENT.md",
    "## Phase 61 EPG Cache Consistency",
    r'''
## Phase 61 EPG Cache Consistency

The real default-backend diagnosis on 2026-07-24 proved that backend-native EPG
IDs can be replaced while previous warm-cache rows remain visible. Phase 61 now
contains a bounded authoritative reconciliation path and a bounded frontend
metadata retry path. Real-system build, restart and browser acceptance remain
required before this slice is marked completed history.
'''
)

append_once(
    "docs/development/current-status.md",
    "## Phase 61 EPG Cache Consistency Work",
    r'''
## Phase 61 EPG Cache Consistency Work

Real-system diagnosis on 2026-07-24 established the concrete cache failure:
RESTfulAPI delivered current IDs `39566`, `39567` and `39568`, while SQLite
still exposed old IDs `38843`, `38844` and `38845` for the same backend/channel
windows. The fix uses authoritative refresh-window reconciliation, preserves
backend scope, does not create a canonical identity from title/time matching,
and leaves the EPG timeline and PR #99 routing untouched.
'''
)

