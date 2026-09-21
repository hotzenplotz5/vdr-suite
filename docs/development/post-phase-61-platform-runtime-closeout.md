# Post-Phase-61 Platform Runtime Closeout

## Status

```text
Completed cross-cutting platform features after Phase 61
PR #110 - VDR Remote mobile interaction hardening
PR #111 - Backend-scoped Global Search
```

These changes are completed implementation on `main`. They are not assigned an invented phase number and do not reopen Phase 61.

## Repository evidence

Baseline used for this closeout:

```text
44ae3102ab202ee0dfc974ee0bc9624b9219ad2d
feat(search): add backend-scoped global search (#111)
```

PR #110 is contained in its parent main history. PR #111 is the merge represented by the baseline commit.

## PR #110 — VDR Remote interaction hardening

The backend-neutral RemoteAction and LiveOverlay contracts originate in PR #99. PR #110 completed the current mobile presentation and action-state behaviour:

- the launcher is labelled `VDR - Fernbedienung`;
- only the pressed key receives pressed/down feedback;
- other keys remain visually raised and are not globally disabled;
- one internal `actionInFlight`/busy guard rejects duplicate dispatch while an action is pending;
- pointer and keyboard press/release states are isolated per button;
- all browser requests use `VdrSuiteClientApi`;
- backend capability and read-only checks remain server-backed;
- RESTfulAPI, SVDRP and other backend transports remain private;
- the existing 35 hotspot actions and vertical-scroll behaviour remain the functional contract.

### Current asset caveat

At this baseline, `main` still serves `vdr-remote-photorealistic.svg`, whose SVG contains an embedded JPEG. Draft PRs #112 and #113 propose competing fixes from an older main base:

- #112 replaces it with a pure SVG rendering;
- #113 replaces it with a direct 360×1220 JPEG.

Neither draft is part of this closeout. Select at most one after rebase, focused tests, installation and real-device acceptance. The interaction contract above must remain unchanged unless a separate reviewed change proves otherwise.

## PR #111 — Backend-scoped Global Search

The implemented request path is:

```text
Frontend search dialog
  -> VdrSuiteClientApi.fetchClientGlobalSearch()
  -> GET /api/search
  -> GlobalSearchApiRuntime
  -> GlobalSearchController
  -> GlobalSearchService
  -> GlobalSearchRepository
  -> existing VDR-Suite SQLite database
```

### Search scope

The first slice searches one explicitly selected backend and covers:

- Recording title and subtitle;
- persisted Recording people;
- EPG title and subtitle;
- persisted EPG people;
- independent Recording/EPG totals and `hasMore` state;
- deterministic backend-scoped ordering and pagination;
- default EPG window from six hours before now through fourteen days after now;
- maximum explicit EPG window of 31 days;
- minimum query length of two folded characters;
- default limit 20 and maximum limit 50 per group.

### Persistence and isolation

Production configuration opens a dedicated SQLite connection with `PRAGMA query_only=ON`. Title and person candidates are produced through set-based query branches and merged deterministically. Normal search GET requests:

- parse no provider JSON;
- call no TVScraper, TMDB, IMDb, RESTfulAPI, SVDRP or SuiteBridge resolver;
- update no search or metadata state;
- reject unknown or disabled backends before repository access.

### Frontend contract

The search dialog provides:

- 280 ms debounce;
- `AbortController` and request-generation stale-response protection;
- a 12-second mobile timeout;
- visible empty, too-short, loading, timeout/error and no-result states;
- grouped Recording and EPG results with optional person summaries;
- Recordings 2 cards and detail owner reuse;
- existing EPG detail owner reuse;
- retained query, result payload and scroll position while details are open.

The EPG timeline and remote hotspot behaviour are unchanged.

### Performance correction

The first mobile test exposed an unacceptably broad EPG search path with 174,164 persisted events. The final implementation separates title/person candidate sets, removes correlated per-event person lookups, avoids provider-JSON parsing and returns count/page data through one statement.

The regression fixture searches 174,164 synthetic EPG events for both `Pulp Fiction` and `John Travolta` under a bounded runtime. Recorded local measurements were about 0.1 seconds for those targeted queries and below 0.8 seconds for a deliberately broad two-character query. These are fixture-specific measurements, not universal latency guarantees.

## Recording-person contract decision

Current main is consistently bounded at:

```text
maximum people: 128
maximum payload: 65,535 bytes
```

The modelled 52-person `Pulp Fiction` payload is preserved, including John Travolta beyond the former twelve-person cutoff. Draft PR #101 raises only plugin-side limits and is therefore not compatible with the current end-to-end contract. A future increase must update plugin, SVDRP transport, backend parser and tests together.

## Validation boundary

PR #110 records JavaScript syntax and remote runtime contract tests. PR #111 records global-search unit/integration/frontend/architecture tests, the 174,164-event regression, documentation tests and install/runtime compatibility checks.

This closeout records merged repository behaviour. The competing remote asset drafts still require separate mobile acceptance and are not silently included.

## Roadmap consequence

These cross-cutting features strengthen the existing platform but do not change the strict next numbered phase:

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```