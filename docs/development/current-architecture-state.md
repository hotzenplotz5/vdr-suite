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

VDR-Suite has moved from direct live RESTfulAPI access per API request toward a daemon-owned snapshot, change-detection, change-feed and runtime-observability architecture.

VDR remains the primary backend domain and source of truth.

RESTfulAPI remains behind the adapter boundary.

API controllers consume service boundaries instead of directly coupling to VDR or RESTfulAPI internals.

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

Live transports in Phase 13 should consume the change feed instead of owning snapshot generation or change detection.

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

Implemented diagnostics endpoint:

```text
GET /api/runtime/diagnostics
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

## Implemented Change Feed Components

- `SnapshotChangeFeedEntry`
- `SnapshotChangeFeed`
- `SnapshotChangeFeedService`
- `SnapshotChangeFeedJsonSerializer`
- `SnapshotChangeFeedController`

---

## Implemented API Areas

Snapshot-backed VDR read APIs:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
```

Snapshot change feed API:

```text
GET /api/vdr/changes
```

Existing application APIs:

```text
GET /api/dashboard
GET /api/jobs
GET /api/recordings
GET /api/metadata
GET /api/runtime/diagnostics
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
- preserve future multi-VDR and permission-aware designs
- preserve future third-party and multi-client API consumers

---

## Next Architecture Step

```text
Phase 13 - Live Update Transport
```

Goal:

Expose snapshot change feed updates through a live transport mechanism.

Candidate transports:

- Server Sent Events
- WebSocket

Rule:

The live transport layer consumes the change feed and must not own snapshot generation, change detection or feed generation.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)