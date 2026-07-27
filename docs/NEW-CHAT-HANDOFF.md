# VDR-Suite New Chat Handoff

## Purpose

This file is the fixed entry point for a new VDR-Suite chat. It records current repository truth, the required verification order and the next strict runtime boundary. Historical phase instructions belong in completed or historical documents, not here.

## Start procedure

Always begin with:

```bash
cd /home/yavdr/vdr-suite
git fetch origin
git status --short --branch
git rev-parse origin/main
git log -1 --oneline origin/main
git switch --create <new-branch> origin/main
```

Do not work directly on `main`. Do not assume that the commit recorded below is still current.

## Time-bound repository baseline

Verified on 2026-07-27:

```text
origin/main
44ae3102ab202ee0dfc974ee0bc9624b9219ad2d
feat(search): add backend-scoped global search (#111)
```

The next chat must fetch and verify `origin/main` independently.

## Required reading order

1. [Current State](CURRENT.md)
2. [Strict Roadmap](planning/roadmap.md)
3. [Phase Map](planning/phase-map.md)
4. [Current Architecture State](development/current-architecture-state.md)
5. [Implementation Dependency Map](planning/implementation-dependency-map.md)
6. [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
7. [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
8. [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)
9. [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
10. [ADR Index](adr/index.md)
11. [Completed Phases](development/completed-phases.md)

## Trust order

Use evidence in this order:

1. current code on `origin/main`;
2. merged PRs and merge commits;
3. tests, architecture checks and packaging guards;
4. recorded real-system acceptance and production measurements;
5. accepted ADRs;
6. current status and roadmap documents;
7. open or old PRs;
8. old handoffs and historical phase plans.

An accepted ADR is a target contract, not proof that its runtime exists. Conversely, an implemented main-code path must not remain labelled Missing or Planned.

## Current position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 is merged, accepted and closed for its defined scope. Do not resurrect `feature/phase61-metadata-genre-browser`, pending repository acceptance or pending real-system acceptance as current work.

## Implemented foundation

Do not describe these areas as wholly missing:

- daemon-owned SQLite and migrations;
- backend registry, backend scope and server-enforced read-only mode;
- snapshots, partial refresh, change feed and SSE foundations;
- status, channels, EPG, Recordings and Timer read paths;
- Recordings 2 as the sole delivered Recording browser;
- guarded Recording rename, move and VDR-trash actions;
- SearchTimer list, discovery, preview, validation and controlled mutation foundations;
- persistent Recording/EPG metadata, people, artwork and Genre read models;
- TVScraper-backed EPG details behind Suite-owned boundaries;
- backend-neutral remote actions and live-overlay snapshots;
- backend-scoped global search;
- modular frontend ownership through `VdrSuiteClientApi`;
- packaging, install staging and real-system acceptance workflows.

## Phase 61 closeout

PR #100 delivered:

- persistent backend-scoped Recording and EPG metadata target bindings;
- persistent people relations;
- canonical and unknown Genre identities;
- provider and derived evidence with active, missing, unknown, stale and conflict states;
- persistent TVScraper media-type and EPG browse-class evidence;
- EPG main classes Film, Serie, Dokumentation and Sport;
- result-backed Film subgenres;
- indexed backend-scoped counts and pages;
- dedicated query-only SQLite read paths;
- asynchronous provider acquisition and provider-failure isolation;
- reuse of Recordings 2 and the existing EPG detail owner;
- unchanged EPG timeline and preserved LiveRemote ownership;
- restart persistence and real-system acceptance.

Normal Genre GET requests and frontend rendering perform no provider resolution.

## Performance closeout

PRs #102 through #108 completed the post-Phase-61 hardening:

- #102: EPG candidate-selection fast path, measured 3.24× on the recorded production fixture;
- #103: architecture guard aligned with strict DVB Film fallback;
- #104: one atomic EPG Genre write transaction per candidate;
- #105: no write transaction for unchanged Recording Genre synchronization;
- #106: selected integer-epoch EPG window index, measured about 19.6× on the recorded production query;
- #107: identical EPG upserts cause no row update or timestamp rewrite;
- #108: completed ETYPES cycles pause 15 minutes, incomplete cursors continue every ten seconds.

Use the exact fixture sizes and qualifications in the closeout. Do not turn a single startup comparison into a general benchmark.

## Recordings 2 ownership

The legacy recording browser is not the delivered runtime owner. Recordings 2 owns:

- folder and recording state;
- cards and details;
- metadata, people, artwork and Genre integration;
- rename, move and trash actions;
- Genre and global-search navigation.

Do not reintroduce the removed legacy scripts or create a second Recording detail owner.

## Remote and live overlay

PR #99 established the backend-neutral RemoteAction and LiveOverlay path. PR #110 established the current mobile key behaviour:

- only the pressed key moves;
- other keys are not disabled during dispatch;
- internal `actionInFlight`/busy state prevents duplicate dispatch;
- the browser uses `VdrSuiteClientApi` only;
- backend protocols remain private.

Current `main` still uses an SVG that embeds a JPEG. Draft PRs #112 and #113 are competing fixes from an older base:

- #112: pure self-contained SVG;
- #113: direct 360×1220 JPEG.

Do not merge both. Rebase the selected approach onto current main and require real-device mobile acceptance while preserving all 35 hotspots, pressed-state, duplicate-dispatch guard and scrolling.

## Global search

PR #111 added the completed backend-scoped first slice:

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

Implemented behaviour includes:

- Recording title/subtitle and persisted person search;
- EPG title/subtitle and persisted person search;
- backend isolation;
- German umlaut/ß folding and Unicode-safe input handling;
- pagination and deterministic ordering;
- Recordings 2 and existing EPG detail navigation;
- retained query, result and scroll state;
- 280 ms debounce, abort/generation stale-response protection and 12-second mobile timeout;
- visible too-short, loading, error and no-result states;
- query-only SQLite reads;
- no provider resolution during a search GET;
- regression coverage with 174,164 synthetic EPG events.

The first slice deliberately searches one selected backend. A future authorized multi-backend aggregator must retain per-backend isolation and bounded pages.

## Recording-person contract

Current main is consistent at:

```text
128 people
65,535 payload bytes
```

The regression model retains all 52 modelled `Pulp Fiction` people, including John Travolta beyond the former twelve-person cutoff. It does not promise that every real provider payload can never exceed 128 people.

Draft PR #101 raises only plugin-side bounds to 256 people and 256 KiB. It conflicts with current SVDRP transport/backend parsing and must not be merged piecemeal. Treat it as superseded/obsolete unless a new versioned end-to-end contract is justified.

## Open PRs at the recorded baseline

| PR | Classification |
| ---: | --- |
| #88 | Old conflicting metadata-image responsiveness draft; re-audit against current code. |
| #101 | Conflicting partial bounds increase; do not merge as-is. |
| #109 | Old-base documentation draft; useful material only, superseded by the truth-refresh replacement. |
| #112 | Competing pure-SVG remote asset draft from old main. |
| #113 | Competing direct-JPEG remote asset draft from old main. |

Always inspect the current PR state again before acting.

## Current architecture rules

- VDR remains native runtime authority.
- VDR-Suite owns external domain, policy, orchestration, persistent read models and client contracts.
- RESTfulAPI, SVDRP, Streamdev, TVScraper and SuiteBridge stay private adapters/providers.
- Browser modules communicate through Suite Client API wrappers.
- Stable Suite identities remain separate from backend-native identities.
- Provider evidence is persisted with provenance and state; providers do not become hidden Suite authority.
- Normal GET paths must remain query-only/provider-free where documented.
- Read-only policy and mutation safety are enforced server-side.
- Target ADRs and current implementation status must remain separate.

Active canonical ADRs run through ADR-0050. ADR-0038 covers metadata/provider strategy; ADR-0039 through ADR-0049 define later control-plane, lifecycle, trust, mutation, job, TimerIntent, provenance, streaming, OSD, API and audit contracts; ADR-0050 defines the domain-repository SQLite boundary.

## Main remaining gaps

- actor identities, authentication sessions, scoped RBAC and centralized authorization;
- append-only accountability and transactional outbox;
- secure Backend Agent enrollment, protected transport, generation, lease and reconnect;
- universal revisions, durable idempotency and verification;
- job claim/retry/compensation/reconciliation;
- TimerIntent, TimerAssignment, scheduler and reconciler;
- authenticated Streaming Gateway;
- isolated legacy OSD compatibility bridge;
- stable `/api/v1`, common errors, ETags and compatibility metadata;
- exact remaining epgsearch edge semantics and broader Live workflow polish;
- later recommendation and knowledge graph.

## Next runtime work

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Begin with actor identity and server-side authorization, then request/correlation context, append-only AccountabilityEvent persistence and a transactional outbox. Do not introduce new remote privileged dispatch before those gates exist.