# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)

---

## Purpose

This roadmap describes the forward direction of VDR-Suite.

Current project status belongs to:

- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)

Completed implementation history belongs to:

- [Completed Phases](../development/completed-phases.md)

---

## Roadmap Principles

VDR-Suite remains:

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- prepared for multi-VDR environments
- suitable for multiple future clients and integrations

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

The runtime performance target is backend workload comparable to established VDR frontends such as `live` when equivalent user-visible information is requested.

---

## Current Position

```text
Completed implementation state
Phase 24.5 - Live RESTfulAPI EPG Verification

Current cleanup
Documentation and roadmap synchronization after Phase 24.5

Next implementation step
Phase 25.0 - EPG REST API Boundary
```

Phase 16 completed the multi-backend polling and runtime context foundation. Backend-aware polling, backend polling coordination, backend runtime contexts, daemon runtime context migration, registry-driven context creation and backend-aware snapshot change feed support are implemented.

Phase 17 completed the initial multi-backend read-side visibility through Phase 17.3. SnapshotAccessService can return all cached backend snapshots, VdrSnapshotReadService exposes multi-backend snapshot lists, VdrSnapshotReadJsonSerializer serializes multi-backend snapshot summaries, `GET /api/vdr/snapshots` exposes these summaries through REST and the route is covered by the API router regression test.

Phase 18 completed opt-in real VDR and RESTfulAPI validation through Phase 18.4. Real RESTfulAPI integration, real snapshot building, real change-state handling, real polling initial snapshot generation and repeated polling stability are validated outside the default fast test set.

Phase 19 completed snapshot change feed validation through Phase 19.3.

Phase 20 completed the live transport foundation through Phase 20.9.

Phase 21.0 documented real VDR runtime polling and EPG performance findings.

Phase 21.1 documented RESTfulAPI event streams as optional backend-specific change hint sources and clarified that event streams provide hints, not authoritative domain payloads.

Phase 21.2 introduced the first selective backend query contract through `VdrEventQuery`, `IVdrAdapter`, `VdrService` and `RestfulApiVdrAdapter`.

Phase 21.3 validated selective RESTfulAPI EPG access against a real VDR.

Phase 22.0 introduced the heavy-domain refresh policy foundation. Events / EPG no longer create automatic full-domain refresh work through `SnapshotRefreshPlanner`.

ADR-0021 documents the long-term selective backend query strategy.

---

## Implemented Foundation Since Phase 14

The following roadmap intentions have moved from planning into implemented foundation work:

```text
Phase 14.8 - BackendRegistry runtime integration
Phase 14.9 - BackendRegistry service layer
Phase 15.0 - BackendRegistry JSON serializer
Phase 15.1 - BackendRegistry controller
Phase 15.2 - BackendRegistry API routing
Phase 15.3 - Backend-aware snapshot read service
Phase 15.4 - Backend-aware VDR controller methods
Phase 15.5 - Backend snapshot API routes
Phase 15.6 - Dynamic backend snapshot route parsing
Phase 15.7 - Multi-snapshot cache foundation
Phase 15.8 - Multi-backend snapshot cache access
Phase 15.9 - Backend-aware snapshot builder
Phase 16.0 - Backend-aware snapshot cache service updates
Phase 16.1 - Backend-aware polling service
Phase 16.2 - Backend polling coordinator
Phase 16.3 - Backend runtime context foundation
Phase 16.4 - Daemon runtime context migration
Phase 16.5 - Coordinator runtime integration
Phase 16.6 - Runtime context collection
Phase 16.7 - Runtime context factory
Phase 16.8 - Runtime context creation from registry
Phase 16.9 - Backend-aware snapshot change feed
Phase 17.0 - Multi-backend snapshot read foundation
Phase 17.1 - Multi-backend snapshot summary serialization
Phase 17.2 - Multi-backend snapshots REST endpoint
Phase 17.3 - Multi-backend REST endpoint tests
Phase 18.0 - Real RESTfulAPI integration validation
Phase 18.1 - Real snapshot builder validation
Phase 18.2 - Real change-state validation
Phase 18.3 - Real polling initial snapshot validation
Phase 18.4 - Real polling stability validation
Phase 19.0 - Snapshot change feed service validation
Phase 19.1 - Polling to change feed runtime validation
Phase 19.2 - Multi-backend change feed aggregation
Phase 19.3 - Snapshot change feed REST validation
Phase 20.0 - LiveUpdateEvent foundation
Phase 20.1 - ILiveTransport
Phase 20.2 - TestLiveTransport
Phase 20.3 - LiveTransportService
Phase 20.4 - LiveTransportFactory
Phase 20.5 - SseLiveTransport
Phase 20.5.1 - ILiveTransport read contract
Phase 20.6 - LiveTransportController
Phase 20.7 - ApiRouter live route
Phase 20.8 - DaemonRuntime live transport wiring
Phase 20.9 - Live transport publish bridge
Phase 21.0 - Real VDR runtime polling findings
Phase 21.1 - RESTfulAPI event stream strategy
Phase 21.2 - Selective event query contract
ADR-0021 - Selective backend query strategy
Phase 21.3 - Selective RESTfulAPI EPG validation
Phase 22.0 - Heavy domain refresh policy
```

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Mid-Term Architecture Roadmap

This section replaces the older Phase 18 - Phase 24 forward-looking roadmap blocks.

Those earlier roadmap blocks now describe completed implementation foundation or superseded direction. Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md), while this file describes the next architecture direction after Phase 24.5.

The mid-term roadmap is organized around the following planned phase groups:

```text
Phase 25.x - EPG REST API Boundary
Phase 26.x - Recording Query Architecture
Phase 27.x - Recording REST API Modernization
Phase 28.x - Recording Actions
Phase 29.x - Recording Metadata and Job Integration
Phase 30.x - Multi-VDR Federation
Phase 31.x - Permission and Role System
Phase 32.x - Advanced Search
Phase 33.x - SearchTimer Architecture
Phase 34.x - Windows Client API Stabilization
```

The following rules guide all phase groups:

- VDR remains the source of truth for channels, timers, EPG, running events, real recordings and device state.
- VDR-Suite does not persist a full EPG mirror in SQLite.
- EPG remains a heavy domain and must be read selectively.
- SQLite is reserved for VDR-Suite-owned application data such as recording metadata, jobs, workflow state, user configuration, permissions, roles and optional search indexes.
- SearchTimer work remains out of scope until the EPG and recording API boundaries are stable.
- Multi-VDR and permission work must build on backend-neutral APIs.
- Frontend-facing APIs must remain backend-aware and client-independent.

Detailed phase sections will be expanded in the next roadmap updates.

---

## Future Product Layers

Possible later layers:

- Web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD integration
- stream provider integration
- media library expansion
- authentication and authorization
- permission-aware backend federation

---

## Roadmap Rule

This file describes direction, not detailed implementation history.

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
