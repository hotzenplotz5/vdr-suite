# VDR-Suite

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. VDR remains the native runtime authority; VDR-Suite owns backend scope, policy, orchestration, persistent read models and client-facing contracts.

## Start here

- [Current State](docs/CURRENT.md)
- [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md)
- [Documentation Index](docs/index.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Completed History](docs/development/completed-phases.md)
- [Phase 61 and Performance Closeout](docs/development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](docs/development/post-phase-61-platform-runtime-closeout.md)
- [VDR Ecosystem Parity](docs/planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](docs/adr/index.md)

## Current verified position

Documentation baseline verified on 2026-07-27 against `origin/main` commit:

```text
44ae3102ab202ee0dfc974ee0bc9624b9219ad2d
feat(search): add backend-scoped global search (#111)
```

This commit is a time-bound reference, not a substitute for fetching and checking the current `origin/main` before starting new work.

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

## Implemented runtime blocks

Current `main` includes:

- daemon-owned SQLite persistence, backend registry, snapshots, change feed and server-enforced read-only backend policy;
- channels, EPG timeline and channel-day programme views;
- Recordings 2 as the sole delivered recording browser, including metadata, people, artwork, Genre integration and guarded actions;
- SearchTimer list, discovery, preview, validation and controlled mutation foundations;
- persistent backend-scoped Recording and EPG metadata, people, Genre evidence, assignments and browse paths;
- the Phase 61 EPG taxonomy for Film, Serie, Dokumentation and Sport, including Film subgenres;
- post-Phase-61 query, transaction, no-op and snapshot-cadence hardening from PRs #102 through #108;
- backend-neutral VDR remote actions and live-overlay snapshots, with isolated pressed-state and duplicate-dispatch protection from PR #110;
- backend-scoped global search over persisted Recording and EPG titles, subtitles and people from PR #111;
- modular frontend ownership through `VdrSuiteClientApi`, without direct browser access to RESTfulAPI, SVDRP, TVScraper or SuiteBridge.

The project is not yet a complete replacement for every Live, epgsearch or RESTfulAPI surface. Streaming, legacy OSD compatibility, production user identity/RBAC, secure Backend Agents, central TimerIntent orchestration, a stable `/api/v1` and universal accountability remain planned work.

## Architecture direction

Accepted ADRs define target contracts; they do not by themselves prove runtime implementation. Current implementation truth comes from `origin/main`, merged PRs, tests and recorded real-system acceptance.

The strict next step is:

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 must establish actor identities, scoped authorization, request context and append-only accountability before later secure Agent-backed privileged operations.