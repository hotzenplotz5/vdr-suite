# Architecture Decision Records (ADR)

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Purpose

This section contains long-term architecture decisions.

Stable architecture descriptions belong in:

- [Architecture Documentation](../architecture/index.md)

Current implementation progress belongs in:

- [Current Project Status](../development/current-status.md)

---

## ADR Numbering Policy

Canonical ADR sequence:

ADR-0001
ADR-0002
...
ADR-0022

Next available ADR:

ADR-0023

The historical lowercase adr-001 to adr-007 files remain for repository history and compatibility.

The legacy numeric files 007 and 008 also remain for repository history and compatibility.

No new ADRs should be created in the lowercase or legacy numeric series.

If a canonical ADR is superseded, keep it only in the superseded section and do not list it as an active canonical decision.

---

## Canonical ADRs

- ADR-0001 Monorepo
- ADR-0002 SQLite
- ADR-0003 REST API
- ADR-0004 C++17
- ADR-0005 External VDR Integration Strategy
- ADR-0006 VDR Backend Architecture
- ADR-0007 RESTfulAPI Adapter Boundary
- ADR-0008 Real HTTP Server Strategy
- ADR-0009 HTTP Server Factory Strategy
- ADR-0010 Library-First VDR Architecture
- ADR-0011 VDR Source Model Architecture
- ADR-0012 Source Capability Model
- ADR-0013 Permission Model
- ADR-0014 Recording Identity Strategy
- ADR-0015 Timer Operation Boundary
- ADR-0016 Snapshot Change Feed Architecture
- ADR-0017 Live Transport Boundary
- ADR-0018 Incremental Snapshot Synchronization
- ADR-0019 SSE Event Stream Transport Strategy
- ADR-0020 Multi-Source Federation Architecture
- ADR-0021 Selective Backend Query Strategy
- ADR-0022 LIVE Functional Reference Strategy

---

## Historical ADR Series

Retained for historical reference:

- adr-001 Backend Identity Strategy
- adr-002 Backend Federation Strategy
- adr-003 Backend Capability Strategy
- adr-004 Backend Lifecycle Strategy
- adr-005 Stream Provider Strategy
- adr-006 Internal Event Dispatch Strategy
- adr-007 Platform API Strategy
- 007 Platform API Strategy
- 008 Runtime Observability Strategy

---

## Superseded ADRs

- [ADR-0011: VDR Source Model](ADR-0011-vdr-source-model.md) superseded by [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)

---

## Related Documents

- [Architecture Documentation](../architecture/index.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Current Project Status](../development/current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Rules

- use the next canonical ADR number
- keep historical ADRs available
- keep superseded ADRs visible
- avoid duplicate active ADR numbers
- link architecture summaries from docs/architecture

## ADR Documents

### Active Canonical ADRs

- [ADR-0001: Monorepo für VDR-Suite](ADR-0001-monorepo.md)
- [ADR-0002: SQLite als zentrale Metadaten-Datenbank](ADR-0002-sqlite.md)
- [ADR-0003: REST API als externe Schnittstelle](ADR-0003-rest-api.md)
- [ADR-0004: C++17 als Mindeststandard](ADR-0004-cpp17.md)
- [ADR-0005: External VDR Integration Strategy](ADR-0005-external-vdr-integration-strategy.md)
- [ADR-0006: VDR Backend Architecture](ADR-0006-vdr-backend-architecture.md)
- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0008: Real HTTP Server Strategy](ADR-0008-real-http-server-strategy.md)
- [ADR-0009: HTTP Server Factory Strategy](ADR-0009-http-server-factory-strategy.md)
- [ADR-0010: Library First VDR Architecture](ADR-0010-library-first-vdr-architecture.md)
- [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0015: Timer Operation Boundary](ADR-0015-timer-operation-boundary.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0019: SSE Event Stream Transport Strategy](ADR-0019-sse-event-stream-transport-strategy.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0021: Selective Backend Query Strategy](ADR-0021-selective-backend-query-strategy.md)
- [ADR-0022: LIVE Functional Reference Strategy](ADR-0022-live-functional-reference-strategy.md)

### Superseded Canonical ADRs

- [ADR-0011: VDR Source Model](ADR-0011-vdr-source-model.md)

### Historical ADRs

- [ADR-001 Backend Identity Strategy](adr-001-backend-identity-strategy.md)
- [ADR-002 Backend Federation Strategy](adr-002-backend-federation-strategy.md)
- [ADR-003 Backend Capability Strategy](adr-003-backend-capability-strategy.md)
- [ADR-004 Backend Lifecycle Strategy](adr-004-backend-lifecycle-strategy.md)
- [ADR-005 Stream Provider Strategy](adr-005-stream-provider-strategy.md)
- [ADR-006 Internal Event Dispatch Strategy](adr-006-internal-event-dispatch-strategy.md)
- [ADR-007: Platform API Strategy](007-platform-api-strategy.md)
- [ADR-008: Runtime Observability Strategy](008-runtime-observability-strategy.md)

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)