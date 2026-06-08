# VDR-Suite – Snapshot Change Feed Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

Status: Proposed for Phase 12.0

This document defines the architecture direction for a read-oriented snapshot change feed.

The goal is to allow future API consumers and frontends to determine which snapshot domains changed without repeatedly polling every snapshot read endpoint.

This architecture document intentionally does not introduce Server-Sent Events, WebSocket transport or frontend coupling.

---

## Current Foundation

The existing implementation already contains the required lower-level building blocks:

- `VdrChangeState` stores domain-level version counters for status, channels, recordings, timers and events.
- `ChangeDetectionService` compares a previous and current `VdrChangeState` and produces domain-level `VdrChangeEvent` values.
- `SnapshotRefreshPlanner` maps change events to a `SnapshotUpdatePlan`.
- `SnapshotCache` stores the current `VdrSnapshot`.
- `SnapshotAccessService` exposes read-only snapshot access through `ISnapshotAccessService`.
- `VdrSnapshotReadService` exposes snapshot-backed read methods for the current domain set.
- `VdrController` and `ApiRouter` expose snapshot read APIs through normal GET routes.

The change feed should build on this foundation without coupling clients to polling internals.

---

## Design Goal

A snapshot change feed is a read-side model that describes what changed between known snapshot states.

It should answer questions such as:

- did anything change since the last observed feed position?
- which snapshot domains changed?
- should a client reload status, channels, timers, events or recordings?
- can future live transports reuse the same feed model?

The feed must remain transport-independent.

---

## Non-Goals for Phase 12.0

Phase 12.0 must not introduce:

- SSE transport
- WebSocket transport
- direct frontend coupling
- browser-specific update behavior
- direct RESTfulAPI client exposure
- polling implementation leakage into public API contracts

---

## Feed Boundary

The snapshot change feed should be an internal read-side boundary above change detection and below future transport layers.

Recommended responsibility split:

```text
VdrChangeState
  -> ChangeDetectionService
  -> VdrChangeEvent
  -> SnapshotRefreshPlanner
  -> SnapshotUpdatePlan
  -> SnapshotChangeFeed model
  -> future REST/SSE/WebSocket consumers
```

The feed should not own snapshot data.

Snapshot data remains owned by `SnapshotCache` and read through `ISnapshotAccessService` / `VdrSnapshotReadService`.

---

## Feed Model Direction

A future feed entry should describe domain-level changes without embedding full snapshot payloads.

Recommended conceptual fields:

- feed sequence number
- snapshot generation or revision
- changed domains
- optional backend/source identity
- optional timestamp
- optional refresh hints

The feed should prefer domain names aligned with existing snapshot read endpoints:

- `status`
- `channels`
- `recordings`
- `timers`
- `events`

---

## Relationship to Existing Read APIs

Existing snapshot read endpoints remain the canonical way to fetch data:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
```

A future change feed endpoint should only tell consumers what changed.

It should not replace these read endpoints.

---

## Multi-VDR and Backend Identity

The change feed must not introduce a permanent single-VDR assumption.

Even if the first implementation remains single-backend internally, the model should leave room for:

- backend identity
- source identity
- future federation
- permission-aware filtering
- per-backend feed positions

This aligns with the existing backend identity, federation, capability and permission architecture direction.

---

## Recommended Phase 12 Implementation Path

Phase 12 should proceed in small architecture-first steps:

1. Define a `SnapshotChangeFeedEntry` domain model.
2. Define a `SnapshotChangeFeed` read model.
3. Define a feed service boundary that consumes `VdrChangeEvent` values.
4. Add tests for feed generation from domain-level change events.
5. Only after the feed model is stable, expose a read-only REST endpoint.
6. Keep SSE/WebSocket work for a later transport phase.

---

## Open Design Questions

Before implementation, decide:

- whether feed sequence numbers are global or per backend
- whether feed retention is bounded in memory
- whether initial feed state should emit a full refresh hint
- whether feed entries should expose raw `VdrChangeType` names or stable API domain names
- how future recording identity changes should align with ADR-0014

---

## Architecture Decision

The snapshot change feed is a read-side architecture layer.

It must remain separate from:

- polling execution
- snapshot cache ownership
- REST transport
- future live transport
- frontend behavior

This keeps the architecture compatible with future REST polling, SSE, WebSocket and multi-VDR scenarios.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
