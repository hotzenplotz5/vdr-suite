# Phase 61 Metadata, Genre and Performance Closeout

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)

---

## Status

```text
Phase 61 - Suite Metadata and Genre Platform
Status: Completed, merged and accepted on the real yaVDR system

Post-Phase 61 Performance Hardening (B1-B4)
Status: Completed, merged and accepted on the real yaVDR system

Next runtime implementation phase
Phase 62 - Identity, RBAC and Accountability Foundation
```

The historical umbrella implementation track remains:

```text
Phase 58 - Frontend and Live Parity
```

It is a product-history grouping and does not override the strict numbered sequence.

---

## Phase 61 Accepted Scope

PR #100 delivered the accepted Phase 61 vertical slice for a Suite-owned metadata and Genre runtime.

The completed path is:

```text
persistent Recording and EPG sources
  -> backend-scoped target bindings
  -> provider and derived evidence
  -> canonical Genre assignments
  -> indexed SQL counts and pagination
  -> provider-neutral Suite REST
  -> DOM-free Web Client API
  -> Genre frontend navigation
  -> existing Recording and EPG detail owners
```

Implemented and accepted contracts include:

- persistent backend-scoped Recording and EPG Genre target bindings;
- canonical aliases, stable unknown identities and explicit unclassified state;
- multiple Genres per Recording or EPG event;
- explicit active, missing, unknown, stale and derived conflict states;
- persistent TVScraper media-type evidence and derived EPG browse classes;
- a four-class EPG entry taxonomy: Film, Serie, Dokumentation and Sport;
- Film subnavigation through canonical Film Genres with actual results;
- indexed distinct counts, backend scoping and limit/offset pagination;
- a dedicated query-only SQLite read connection for Genre GET paths;
- bounded asynchronous EPG enrichment through the daemon cache worker;
- Recording Genre materialization through the Recording cache worker;
- provider-neutral REST and Web Client API surfaces;
- reuse of the existing Recording and EPG detail owners;
- unchanged EPG timeline ownership and preserved LiveRemote routing precedence;
- restart persistence and real-system acceptance.

Phase 61 does not make TVScraper, RESTfulAPI or another external database authoritative for VDR-Suite. Provider acquisition remains replaceable; Suite-owned persisted assignments and read models are the application boundary.

---

## Phase 61 Completion Boundary

The phase is closed for the accepted metadata/Genre platform scope above.

The following are not claimed as fully implemented by Phase 61:

- a universal metadata entity model for every future media domain;
- every possible sidecar, import or external provider adapter;
- a complete derivative artwork processing farm;
- long-term provider job history and percentile diagnostics;
- recommendation or knowledge-graph behavior.

Those are later extensions or future backlog. They do not keep the accepted Phase 61 runtime phase open.

---

## Post-Phase 61 Performance Hardening

After PR #100, production measurements identified avoidable EPG query work, writer transactions, unchanged row updates and repeated TVScraper type-snapshot scans. PRs #102 through #108 closed those findings.

### B1 - EPG refresh candidate query

PR #102 added a ten-digit epoch fast path while retaining the legacy/synthetic fallback.

Measured on the production database:

```text
old median: 1322.374 ms
new median:  408.584 ms
speedup:          3.24x
reduction:        69.1%
result difference: none
```

### B2a - Atomic EPG Genre evidence writes

PR #104 combined TVScraper Genre evidence, media-type evidence and derived browse-class reconciliation into one transaction per candidate.

For a 64-candidate batch:

```text
before: up to 192 BEGIN IMMEDIATE transactions
after:  up to  64 BEGIN IMMEDIATE transactions
```

### B2b - Recording Genre synchronization no-op

PR #105 compares the desired assignment signature with persisted evidence before opening a write transaction.

Production inventory used for validation:

```text
cached recordings:            1003
Recording Genre assignments:  1777
unchanged synchronization:    no BEGIN, COMMIT, INSERT, UPDATE or DELETE
```

### B3a - Integer EPG window index

PR #106 added the selected SQLite expression index for integer EPG end-time predicates.

Production measurement:

```text
EPG rows:          170631
baseline median: 1019.063 ms
indexed median:    52.014 ms
speedup:               19.6x
result rows:          12861 on both paths
```

A slower start-time expression-index candidate was deliberately not added.

### B3b - Unchanged EPG upsert suppression

PR #107 added a conditional conflict-update predicate so identical events keep their original `updated_at` value and produce no SQLite row change.

Live validation after daemon restart:

```text
common rows:                         170871
unchanged + timestamp preserved:    170858
unchanged + timestamp rewritten:         0
changed + timestamp updated:            13
changed + timestamp unchanged:           0
```

The first observed startup comparison completed in about 65 seconds versus a prior roughly 73-second run. This is a single operational comparison, not a formal benchmark claim.

### B4 - TVScraper type-snapshot restart throttling

PR #108 retained the ten-second continuation cadence for incomplete snapshots but delayed a new completed snapshot cycle for 15 minutes.

Live acceptance:

```text
first snapshot complete: 17:20:39
scanned offset:              13411
applied:                      true
snapshot pages in next 120 s:   0
next periodic cycle:       17:35:45
observed idle interval:      15 min 6 s
```

The six-second difference is consistent with the periodic worker cadence.

---

## Merge Record

| PR | Result |
| ---: | --- |
| #100 | Persistent metadata-backed Genre browser and Phase 61 runtime slice. |
| #102 | Faster EPG enrichment candidate query. |
| #103 | Architecture checker aligned with the accepted strict DVB fallback. |
| #104 | Atomic EPG Genre evidence writes. |
| #105 | Unchanged Recording Genre synchronization skipped. |
| #106 | Integer EPG window index and query-plan regression. |
| #107 | Unchanged EPG event upserts skipped. |
| #108 | Completed ETYPES snapshot cycles throttled to 15 minutes. |

Final production main after the closeout measurements:

```text
51a67fb2 Merge pull request #108
f6b277c8 perf(epg): throttle completed type snapshots
```

---

## Acceptance Evidence

The accepted runtime evidence includes:

- repository-focused Genre and EPG tests;
- architecture contract checks;
- daemon builds and runtime installation;
- SQLite restart persistence;
- live Recording and EPG Genre counts;
- TVScraper media-type snapshot pagination;
- no-op update verification through SQLite timestamps and change counts;
- production query-plan and timing comparisons;
- daemon restart and journal validation;
- 15-minute completed-snapshot idle-window validation;
- clean `main == origin/main` after PR #108;
- active installed daemon on the real yaVDR system.

---

## Operational Lessons

1. Large authoritative refreshes must distinguish fetched data from actual changes.
2. A correct no-op path is as important as a fast changed-data path.
3. SQLite query plans must be verified on production-shaped timestamps and row counts.
4. Bounded pagination does not by itself prevent continuous full rescans.
5. Runtime acceptance must check cadence and idle behavior, not only successful completion.
6. Provider evidence may be refreshed asynchronously, but HTTP reads and frontend rendering must remain provider free.

---

## Next Boundary

The next runtime implementation phase is:

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 should begin with actor identity and centralized authorization contracts before migrating privileged operations. Append-only accountability evidence and a protected outbox must exist before later Agent-backed or remote privileged dispatch.

The current comparison with VDR Core, Live, epgsearch and RESTfulAPI is maintained in [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md).
---

## Back

- [Back to Completed Phases](completed-phases.md)
- [Back to Current State](../CURRENT.md)
- [Back to Development Index](index.md)
