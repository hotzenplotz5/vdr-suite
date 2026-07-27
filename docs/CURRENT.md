# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Android, Android TV and Client API Feasibility Study](architecture/android-client-api-feasibility-study.md)
- [Client Capability, API Candidate and Gap Matrix](planning/client-capability-api-gap-matrix.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Completed Phases](development/completed-phases.md)
- [ADR Index](adr/index.md)

## Repository baseline

This document was reconciled on 2026-07-27 against:

```text
origin/main
cb77ff66e11dca7db2eafa36525762dcde35102d
Merge pull request #115 from hotzenplotz5/agent/configurable-remote-mapping
```

PR #114 is merged at `729ff190c532b9cd59c6b7054cd52062409af26c`. The SHA above is a time-bound evidence point. Every new task must fetch `origin/main`, determine the current head and branch from that actual commit.

## Current verified position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 is merged and closed for its accepted metadata/Genre scope. It is not an active feature branch and no Phase 61 repository or real-system acceptance remains pending.

The Android/client feasibility study is cross-cutting planning evidence for Phases 62 through 67. It does not create a new runtime phase and does not change Phase 62 as the next strict step.

## Implemented runtime truth

Current `main` contains:

- daemon-owned SQLite persistence, migrations and domain repository boundaries;
- BackendNode/BackendRegistry, backend-scoped snapshots, change feed, SSE foundations and server-enforced read-only backend policy;
- channels, current-programme and channel-day views, persistent EPG cache and the existing EPG timeline;
- Recordings 2 as the sole delivered recording browser, including folders, cards, detail view, metadata, people, artwork, Genre integration and guarded rename/move/trash actions;
- SearchTimer list, discovery, preview, validation, native capability handling and controlled mutation foundations;
- persistent backend-scoped Recording and EPG metadata identities, people relations, Genre evidence, assignment states and query-only browse paths;
- the accepted EPG hierarchy Film, Serie, Dokumentation and Sport, plus result-backed Film subgenres;
- asynchronous provider acquisition with provider-failure isolation and no provider resolution from normal Genre or search GET requests;
- backend-neutral remote actions and live-overlay snapshots through Suite-owned API and Client API boundaries;
- backend-scoped global search over persisted Recording and EPG titles, subtitles and people;
- the merged 360×1220 transparent PNG remote with 35 hotspots, help/assignment view and current mobile mappings;
- packaging, install staging, daemon builds and real-system acceptance workflows.

Current `main` does **not** contain a Suite video player, Streaming Gateway or MediaSession runtime. The browser LiveOverlay is structured channel/programme state, not a video stream or legacy OSD frame plane.

## Phase 61 closeout

PR #100 delivered the completed metadata-backed Genre vertical slice. The public read path is:

```text
persistent Recording/EPG sources
  -> backend-scoped bindings and people relations
  -> provider and derived evidence
  -> canonical Genre assignments and states
  -> query-only indexed SQLite reads
  -> Suite REST
  -> VdrSuiteClientApi
  -> Genre navigation
  -> existing Recordings 2 and EPG detail owners
```

The EPG timeline remained unchanged. Provider acquisition stays asynchronous and private. See [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md).

## Post-Phase-61 performance hardening

PRs #102 through #108 completed:

- a ten-digit epoch fast path for EPG enrichment candidates;
- architecture-contract alignment with the accepted strict DVB fallback;
- one atomic write transaction per EPG Genre candidate;
- no writer transaction for unchanged Recording Genre synchronization;
- the selected integer-epoch EPG window index;
- no-op suppression for unchanged EPG event upserts;
- a 15-minute pause between completed ETYPES cycles while incomplete cursors continue on the ten-second cadence.

The recorded production measurements and their limits are maintained in the Phase 61 closeout; isolated timings must not be generalized beyond those fixtures.

## Remote and live overlay

PR #99 established the backend-neutral RemoteAction and LiveOverlay contracts. PR #110 completed the pressed-state/busy-guard mobile behaviour. PR #115 then merged the current configurable photorealistic remote:

- transparent 360×1220 PNG asset;
- unchanged 35-hotspot geometry;
- narrow centred mobile presentation;
- image preload;
- HOME mapped to the VDR menu;
- VDR-Suite logo mapped to overview;
- EPG mapped to the timeline;
- contextual REC start/stop workflow;
- help and assignment dialog with German/English text;
- browser traffic remains inside `VdrSuiteClientApi`;
- RESTfulAPI, SVDRP and other backend protocols remain private.

The current overlay path is:

```text
GET /api/vdr/live/overlay
  -> LiveRemoteApiRuntime
  -> structured channel / present / following / timer state

GET /api/vdr/live
  -> sequenced SSE changed-domain notifications
```

Neither endpoint carries video bytes. No browser `<video>` element or Suite media route is implemented. Draft PR #112 is an obsolete old-base pure-SVG alternative and must not replace the merged PNG runtime without separate new evidence.

## Backend-scoped global search

PR #111 added:

```text
Frontend
  -> VdrSuiteClientApi
  -> GET /api/search
  -> GlobalSearchApiRuntime
  -> GlobalSearchController
  -> GlobalSearchService
  -> GlobalSearchRepository
  -> existing VDR-Suite SQLite database
```

The search supports persisted Recording and EPG titles, subtitles and people, backend isolation, German folding, pagination, deterministic ordering, Recordings 2 and existing EPG detail navigation, retained query/result/scroll state, debounce, stale-response protection, visible loading/error/no-result states and a 12-second mobile timeout. A production-shaped regression uses 174,164 EPG events. Normal search GETs use a query-only connection and perform no provider lookup. See [Backend-Scoped Global Search](architecture/global-search.md).

## Recording-person contract

Current `main` has one consistent bounded contract:

```text
maximum people: 128
maximum RMETA payload: 65,535 bytes
```

The regression model retains all 52 modelled `Pulp Fiction` people, including John Travolta beyond the former twelve-person cutoff. This proves that modelled 52-person completeness, not universal completeness for every possible provider payload.

Draft PR #101 raises only plugin-side limits to 256 people and 256 KiB. It conflicts with the current transport/parser contract and must not be merged without one coordinated versioned change across plugin, SVDRP transport, backend parser and tests. In its current form it should be treated as superseded or closed obsolete.

## Pull request classification at this baseline

| PR | Status in repository truth |
| ---: | --- |
| #69 | Old CI optimisation draft; re-evaluate against current workflow before reuse. |
| #78 | Old stacked channel/day frontend draft; do not treat as current UI truth. |
| #88 | Old, conflicting metadata-image responsiveness draft; re-evaluate against current listener/runtime before any reuse. |
| #101 | Conflicting partial person-limit increase; do not merge as-is. |
| #109 | Closed unmerged as superseded by merged PR #114. |
| #112 | Obsolete competing pure-SVG remote proposal from an old main base. |
| #113 | Superseded by merged PR #115. |
| #114 | Merged documentation repository-truth refresh at `729ff190...`. |
| #115 | Merged configurable photorealistic remote; current main head at this baseline. |

Open PR content remains lower-trust than current `main` and merged evidence.

## Accepted target contracts versus implementation

Active ADRs run through ADR-0050. ADR-0038 defines metadata/provider ownership. ADR-0039 through ADR-0049 define later Agent, lifecycle, trust, mutation, jobs, TimerIntent, provenance, streaming, OSD, API and audit contracts. ADR-0050 reinforces the domain-repository SQLite boundary.

Accepted target contracts do not mean the corresponding runtime is complete.

## Main remaining gaps

- actor identities, sessions, scoped RBAC and centralized authorization;
- append-only accountability and a transactional outbox;
- secure Backend Agent enrollment, generation, lease, reconnect and fenced commands;
- universal revisions, durable idempotency and mutation verification;
- production job claim/retry/reconciliation semantics;
- TimerIntent, TimerAssignment, scheduler and reconciler;
- authenticated Streaming Gateway and media sessions;
- isolated legacy OSD view/control bridge;
- stable `/api/v1`, common errors, ETags and compatibility metadata;
- exact remaining epgsearch edge semantics and broader Live workflow parity;
- later recommendation and knowledge-graph work.

## Immediate work

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 begins with actor identity, scoped authorization, request/correlation context and append-only accountability. No later Agent-backed privileged dispatch may bypass those gates.

## Boundary rules

- VDR remains native runtime authority.
- VDR-Suite owns external domain, policy, orchestration, persistent read models and client contracts.
- Browsers, Android clients and third-party clients do not call RESTfulAPI, SVDRP, Streamdev, TVScraper or SuiteBridge directly.
- Provider data is evidence, not hidden Suite authority.
- Normal GET/read rendering does not trigger provider resolution.
- Frontends do not own authorization decisions.
- Completed phases are not silently reopened by optional extensions.
- The JavaScript wrapper, public HTTP API, Android SDK, Agent protocol and Media Plane are separate contracts.
