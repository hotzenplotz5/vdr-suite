# VDR-Suite Current Architecture State

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Architecture Index](../architecture/index.md)

---

## Purpose

This document summarizes the current implemented architecture state of VDR-Suite.

It complements:

- [Current Project Status](current-status.md)
- [Architecture Documentation](../architecture/index.md)
- [Roadmap](../planning/roadmap.md)

---

## Current Architecture Direction

VDR-Suite has moved from direct live RESTfulAPI access per API request toward a daemon-owned snapshot, change-detection, change-feed, backend registry and runtime-observability architecture.

VDR remains the primary backend domain and source of truth.

RESTfulAPI remains behind the adapter boundary.

API controllers consume service boundaries instead of directly coupling to VDR or RESTfulAPI internals.

The current architecture is prepared for multi-backend read routing. Multi-backend polling is the next runtime step.

---

## Current Runtime Chain

```text
VDR
    ↓
RESTfulAPI
    ↓
BasicHttpClient
    ↓
RestfulApiVdrAdapter
    ↓
VdrService
    ↓
VdrSnapshotBuilder
    ↓
PollingService
    ↓
ChangeDetectionService
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotCacheService
    ↓
SnapshotCache
    ↓
SnapshotAccessService
    ↓
VdrSnapshotReadService
    ↓
VdrSnapshotReadJsonSerializer
    ↓
VdrController
```

---

## Current Backend Registry Chain

```text
BackendNode
    ↓
BackendRegistry
    ↓
BackendRegistryService
    ↓
BackendRegistryJsonSerializer
    ↓
BackendRegistryController
    ↓
ApiRouter
    ↓
GET /api/backends
GET /api/backends/{backendId}
```

The registry foundation is implemented for read-side and routing preparation.

---

## Current Backend-Aware Snapshot Chain

```text
BackendRegistry
    ↓
VdrSnapshotBuilder
    ↓
VdrSnapshot.backendId
    ↓
SnapshotCache.updateForBackend(...)
    ↓
SnapshotAccessService.snapshotForBackend(...)
    ↓
VdrSnapshotReadService
    ↓
VdrController backend-aware methods
    ↓
/api/backends/{backendId}/...
```

The cache and access boundaries are ready for multiple backend snapshots while preserving default single-backend compatibility.

---

## Current Change Feed Chain

```text
SnapshotCache
    ↓
SnapshotChangeFeedService
    ↓
SnapshotChangeFeed
    ↓
SnapshotChangeFeedJsonSerializer
    ↓
SnapshotChangeFeedController
    ↓
GET /api/vdr/changes
```

The change feed is transport-independent.

Live transports remain future work and should consume the change feed instead of owning snapshot generation or change detection.

---

## Current Runtime Diagnostics Chain

```text
BasicHttpClient
VdrSnapshotBuilder
PollingService
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnosticsJsonSerializer
    ↓
RuntimeDiagnosticsController
```

Implemented diagnostics endpoints:

```text
GET /api/runtime/diagnostics
GET /api/runtime/summary
```

Runtime diagnostics remain separate from logging.

---

## Implemented Snapshot Components

- `VdrSnapshot`
- `VdrSnapshotBuilder`
- `PollingService`
- `VdrChangeState`
- `VdrChangeEvent`
- `ChangeDetectionService`
- `SnapshotRefreshDecision`
- `SnapshotRefreshDecisionService`
- `SnapshotUpdatePlan`
- `SnapshotRefreshPlanner`
- `SnapshotCache`
- `SnapshotCacheService`
- `ISnapshotAccessService`
- `SnapshotAccessService`
- `VdrSnapshotReadService`
- `VdrSnapshotReadJsonSerializer`

---

## Implemented Backend Registry Components

- `BackendNode`
- `BackendRegistry`
- `BackendRegistryService`
- `BackendRegistryJsonSerializer`
- `BackendRegistryController`

---

## Implemented Change Feed Components

- `SnapshotChangeFeedEntry`
- `SnapshotChangeFeed`
- `SnapshotChangeFeedService`
- `SnapshotChangeFeedJsonSerializer`
- `SnapshotChangeFeedController`

---

## Implemented API Areas

Snapshot-backed default VDR read APIs:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/snapshot
GET /api/vdr/capabilities
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
GET /api/vdr/changes
```

Backend registry and backend-aware read APIs:

```text
GET /api/backends
GET /api/backends/default
GET /api/backends/{backendId}/status
GET /api/backends/{backendId}/health
GET /api/backends/{backendId}/snapshot
```

Existing application APIs:

```text
GET /api/dashboard
GET /api/jobs
GET /api/recordings
GET /api/metadata
GET /api/runtime/diagnostics
GET /api/runtime/summary
```

---

## Architecture Constraints

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- keep refresh decisions inside daemon-owned services
- keep snapshot storage separate from polling logic
- keep snapshot read services separate from cache ownership
- keep change feed generation separate from live transport
- keep logging and diagnostics separate
- keep API controllers backend-independent
- preserve default-backend compatibility
- preserve future multi-VDR and permission-aware designs
- preserve future third-party and multi-client API consumers

---

## Next Architecture Step

```text
Phase 16.0 - Multi-Backend Polling Foundation
```

Goal:

Connect `BackendRegistry` to daemon-owned polling and backend-aware snapshot cache updates.

Rules:

- polling remains daemon-owned
- snapshot generation remains backend-neutral
- default backend behavior remains compatible
- no parallel polling until the single-threaded multi-backend model is clear
- frontend routes must not own polling, backend selection or snapshot generation

---

### Capability Resolver Foundation

The VDR capability layer has an implemented foundation:

- `VdrCapabilitySet`: capability data
- `ICapabilityResolver`: resolver boundary
- `CapabilityResolver`: supports(capability)

`GET /api/vdr/capabilities` is wired through `CapabilityResolver`.

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
