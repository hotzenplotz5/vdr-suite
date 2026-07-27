# VDR-Suite Current Project Status

## Current verified position

Baseline reconciled on 2026-07-27 against `origin/main` commit `44ae3102ab202ee0dfc974ee0bc9624b9219ad2d`.

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

## Stable implemented scope

- daemon-owned SQLite, migrations, repositories and backend registry;
- backend-scoped snapshots, change feed, SSE foundation and server-enforced read-only mode;
- channels, EPG timeline, channel-day view, Recording and Timer read paths;
- Recordings 2 as the sole delivered Recording browser with metadata, people, artwork, Genres and guarded actions;
- SearchTimer list, preview, validation and controlled mutation foundations;
- persistent Recording/EPG metadata, people and Genre read models;
- Phase 61 EPG taxonomy and Film subgenres;
- query-only provider-free Genre and global-search GET paths;
- backend-neutral RemoteAction and LiveOverlay paths;
- isolated remote pressed-state and duplicate-dispatch guard from PR #110;
- backend-scoped global search from PR #111, including the 174,164-event regression fixture;
- modular frontend Client API ownership and install/runtime staging.

## Completed evidence

- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
- [Completed Phases](completed-phases.md)

## Open limitations

The platform is not yet complete for production user identity/RBAC, append-only accountability, secure remote Backend Agents, universal revision/idempotency, TimerIntent orchestration, streaming, legacy OSD compatibility, stable `/api/v1` contracts or exact full epgsearch/Live parity.

The current remote asset itself remains under competing Draft PRs #112 and #113; the merged interaction contract from #110 is complete and must remain stable while one asset approach is separately selected and mobile-tested.

The recording-person contract remains 128 people and 65,535 bytes. Draft PR #101 is a conflicting plugin-only increase and is not current runtime truth.

## Immediate implementation focus

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 must establish actor identity, scoped server-side authorization, request/correlation context, append-only accountability and a transactional outbox before later Agent-backed privileged dispatch.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Current Architecture State](current-architecture-state.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)