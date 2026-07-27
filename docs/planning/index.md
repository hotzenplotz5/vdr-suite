# Planning Documentation

## Purpose

This section contains genuinely open work, strict dependency order and living gap registers. Completed implementation belongs in development closeouts/history; stable architecture belongs in architecture/ADRs.

## Authoritative planning documents

### Execution order

- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)

### Architecture and domain dependencies

- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Android, Android TV and Client API Feasibility Study](../architecture/android-client-api-feasibility-study.md)
- [Client Capability, API Candidate and Gap Matrix](client-capability-api-gap-matrix.md)
- [Domain Dependency Map](domain-dependency-map.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [ADR Index](../adr/index.md)

### Product and ecosystem parity

- [VDR Ecosystem Parity and Product Gaps](parity-audit-and-frontend-gap-roadmap.md)
- [Post-Phase-61 Provider Strategy](tvscraper-recording-metadata-roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)

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

Phase 61, B1-B4 and the listed post-phase platform features are completed implementation and therefore must not remain in active planning sections except as prerequisites or completion context.

The Android/client feasibility work is a cross-cutting study and gap register. It does not insert a phase before Phase 62.

## Strict future sequence

```text
Phase 62 - Identity, RBAC and Accountability Foundation
Phase 63 - Backend Agent and Secure Multi-Site Runtime
Phase 64 - Timer Intent and Multi-Backend Orchestration
Phase 65 - Streaming Gateway and Media Sessions
Phase 66 - Legacy OSD Compatibility Bridge
Phase 67 - Public API and Client Compatibility Hardening
Phase 68 - Recommendation and Content Knowledge Graph
```

## Status rules

- **CURRENT** is based on main code, merged PRs, tests and real-system evidence.
- **PLANNED** contains only work that is not implemented.
- **COMPLETED** moves to development history/closeouts.
- **HISTORICAL** material remains traceable but is not a current entry point.
- **SUPERSEDED** material points to its replacement.
- **DEFERRED** work names its prerequisites and later owner.
- Accepted ADRs change target contracts, not automatically implementation status.

## Current planning cautions

- PR #109 was closed on 2026-07-27 as superseded by merged PR #114; it remains historical source material only.
- Draft PR #101 is not an end-to-end compatible person-limit change.
- PR #115 is merged and supersedes the earlier direct-JPEG proposal #113. Draft PR #112 remains an obsolete competing old-base pure-SVG proposal and must not replace the merged PNG runtime without new evidence.
- Provider additions are post-Phase-61 strategy/backlog, not unfinished Phase 61 slices.
- Current unversioned browser routes and JavaScript wrappers are not automatically the future public API.

## Completed evidence used as planning prerequisites

- [Phase 61 Metadata, Genre and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](../development/post-phase-61-platform-runtime-closeout.md)
- [Completed Architecture Source Audit](../development/architecture-source-audit-2026-07-15.md)
- [Completed Phases](../development/completed-phases.md)

## Historical and superseded planning evidence

- [Repository-truth refresh archive](history/repository-truth-refresh-2026-07/README.md)

Historical snapshots are retained for traceability but are not active execution order.

## Related current documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](../development/current-status.md)
- [Current Architecture State](../development/current-architecture-state.md)
