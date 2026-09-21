# Phase 61 Metadata, Genre and Performance Closeout

## Status

```text
Phase 61 - Suite Metadata and Genre Platform
Status: Completed, merged and accepted

Post-Phase 61 Performance Hardening (B1-B4)
Status: Completed and merged
```

This document records the completion boundary for PR #100 and the measured hardening delivered by PRs #102 through #108. Later remote and global-search work is recorded separately in [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md).

## Accepted Phase 61 scope

PR #100 completed the first Suite-owned persistent metadata/Genre vertical slice:

```text
persistent Recording and EPG sources
  -> backend-scoped target bindings and people relations
  -> provider and derived evidence
  -> canonical Genre identities and assignments
  -> explicit active/missing/unknown/stale/conflict states
  -> indexed query-only counts and result pages
  -> Suite REST
  -> VdrSuiteClientApi
  -> Genre frontend navigation
  -> existing Recordings 2 and EPG detail owners
```

Implemented scope includes:

- persistent backend isolation for Recordings and EPG events;
- normalized canonical aliases, stable unknown identities and unclassified state;
- multiple Genres per target and retained original provider values;
- separate TVScraper media-type and derived EPG browse-class evidence;
- exactly four EPG main classes: Film, Serie, Dokumentation and Sport;
- Film subgenres returned only when matching classified results exist;
- bounded asynchronous provider enrichment;
- provider-failure isolation and stale evidence retention;
- query-only SQLite reads for normal Genre GET requests;
- no provider lookup from HTTP GET or frontend rendering;
- unchanged EPG timeline;
- reuse of Recordings 2 and the existing EPG detail owner;
- restart persistence, backend isolation and real-system acceptance.

## Completion boundary

Phase 61 does not claim:

- every possible metadata provider or import adapter;
- universal cross-provider programme identity;
- a derivative artwork processing farm;
- long-term provider-job percentile diagnostics;
- recommendation or knowledge-graph behaviour.

Those items are later backlog or explicitly planned phases. They do not keep Phase 61 open.

## Post-Phase-61 performance hardening

### B1 — EPG refresh candidate selection (#102)

A ten-digit epoch fast path retains the legacy/synthetic fallback and returned equivalent candidates on the recorded production window.

```text
old median: 1322.374 ms
new median:  408.584 ms
speedup:          3.24x
reduction:        69.1%
result difference: none
```

### Architecture contract correction (#103)

The Genre architecture guard was aligned with schema version 9 and the accepted strict DVB fallback. This changed a stale test contract, not runtime behaviour.

### B2a — Atomic EPG Genre writes (#104)

TVScraper Genre evidence, media-type evidence and derived browse-class reconciliation now share one transaction per candidate.

```text
64-candidate batch before: up to 192 BEGIN IMMEDIATE transactions
64-candidate batch after:  up to  64 BEGIN IMMEDIATE transactions
```

### B2b — Recording Genre no-op synchronization (#105)

Desired evidence is compared with persisted assignment signatures before a writer transaction begins.

```text
cached recordings:           1,003
Genre assignments:           1,777
unchanged refresh:            no BEGIN/COMMIT/INSERT/UPDATE/DELETE
```

### B3a — Integer EPG window index (#106)

The selected SQLite expression index covers integer end-time predicates while preserving existing timestamp semantics.

```text
EPG rows:          170,631
baseline median:  1019.063 ms
indexed median:     52.014 ms
speedup:               ~19.6x
result rows:          12,861 on both paths
```

A slower start-time expression-index candidate was deliberately not added.

### B3b — Unchanged EPG upsert suppression (#107)

Identical events no longer rewrite persisted fields or `updated_at`; changed events still update.

Recorded live comparison:

```text
common rows:                       170,871
unchanged + timestamp preserved:  170,858
unchanged + timestamp rewritten:        0
changed + timestamp updated:           13
changed + timestamp unchanged:          0
```

The observed startup comparison of about 65 seconds versus a prior roughly 73 seconds is one operational observation, not a general benchmark.

### B4 — Completed ETYPES snapshot throttling (#108)

Incomplete cursors continue every ten seconds. A new periodic cycle starts only after a completed snapshot has been idle for 15 minutes; startup and dirty-refresh paths remain immediate.

Recorded acceptance:

```text
first completion:          17:20:39
next periodic cycle:       17:35:45
observed idle interval:      15 min 6 s
```

## Merge record

| PR | Result |
| ---: | --- |
| #100 | Persistent metadata-backed Genre browser and Phase 61 runtime. |
| #102 | Faster EPG enrichment candidate query. |
| #103 | Architecture guard aligned with accepted DVB fallback. |
| #104 | Atomic EPG Genre evidence writes. |
| #105 | Unchanged Recording Genre synchronization skipped. |
| #106 | Integer EPG window index and query-plan regression. |
| #107 | Unchanged EPG event upserts skipped. |
| #108 | Completed ETYPES cycles throttled to 15 minutes. |

The historical closeout head after PR #108 was `51a67fb2`. Later main commits #110 and #111 do not change this Phase 61 completion boundary.

## Acceptance evidence

The completion record includes focused repository tests, architecture guards, daemon build/install, SQLite restart persistence, real Recording and EPG counts, query-plan/timing comparisons, timestamp/no-op checks, daemon journal checks and real-system browser acceptance.

## Operational lessons

- Correct no-op paths are as important as fast changed-data paths.
- Query plans must be checked with production-shaped timestamps and row counts.
- Bounded pagination does not by itself prevent continuous rescans.
- Provider enrichment may run asynchronously, but normal reads remain provider-free.
- Runtime acceptance must verify idle cadence and restart behaviour, not only successful completion.

## Next boundary

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 must establish actor identity, centralized authorization and append-only accountability before later Agent-backed privileged dispatch.