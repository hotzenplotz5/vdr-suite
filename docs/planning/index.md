# Planning Documentation

## Purpose

This section contains genuinely open work, strict dependency order and living gap registers. Completed implementation belongs in development closeouts/history; stable architecture belongs in architecture/ADRs.

## Authoritative planning documents

### Execution order

- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)

### Completed Phase 62 references

- [Phase 62 Final Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)

### Architecture and domain dependencies

- [Target Platform Architecture](../architecture/target-platform-architecture.md)
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
Phase 62 - Identity, RBAC and Accountability Foundation

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active runtime phase:
none; Phase 63 is planned but not started
```

## Strict future sequence

```text
Phase 63 - Backend Agent and Secure Multi-Site Runtime
Phase 64 - Timer Intent and Multi-Backend Orchestration
Phase 65 - Streaming Gateway and Media Sessions
Phase 66 - Legacy OSD Compatibility Bridge
Phase 67 - Public API and Client Compatibility Hardening
Phase 68 - Recommendation and Content Knowledge Graph
```

## Phase 62 planning closeout

Phase 62 is completed through Slice 2X. The final runtime proof is recorded in the Slice-2X closeout and durable evidence directory.

Compatibility retirement was explicitly evaluated. Legacy Basic remains transitional because the code and packaged deployment defaults do not yet mandate migration to `enforced`. Removal requires a separate future migration contract and does not reopen Phase 62.

No audit product, generic security administration, generic Outbox, universal idempotency framework or native/service credential lifecycle is automatically carried into Phase 63.

## Status rules

- **CURRENT** is based on code, tests and real-system evidence, with active branch work identified separately.
- **PLANNED** contains only work that is not implemented.
- **COMPLETED** moves to development history/closeouts.
- **HISTORICAL** material remains traceable but is not a current entry point.
- **SUPERSEDED** material points to its replacement.
- **DEFERRED** work names its prerequisites and later owner.
- Accepted ADRs change target contracts, not automatically implementation status.

## Current planning cautions

- Phase 62 is completed and must not be reopened without a new necessity proof.
- PR #117 remains open, Draft and unmerged pending explicit approval.
- Phase 63 has not started and requires its own bounded contract.
- Android and client architecture are consumers of security contracts, not automatically Phase-63 implementation scope.
- Provider additions are post-Phase-61 strategy/backlog, not unfinished Phase-61 or Phase-62 slices.

## Completed evidence used as planning prerequisites

- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Phase 61 Metadata, Genre and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](../development/post-phase-61-platform-runtime-closeout.md)
- [Completed Phases](../development/completed-phases.md)

## Historical and superseded planning evidence

- [Repository-truth refresh archive](history/repository-truth-refresh-2026-07/README.md)

## Related current documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](../development/current-status.md)
- [Current Architecture State](../development/current-architecture-state.md)
