# VDR-Suite – Current Architecture State

This document contains the current architecture state that was split out of `docs/development/current-status.md` during Phase 10.21.1.

---

## Current Architecture State

VDR-Suite is moving from direct live RESTfulAPI access per API request toward a daemon-owned snapshot, change-detection and runtime-observability architecture.

Current implemented and locally validated runtime chain:

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
VdrChangeEvent
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
SnapshotCacheService
    ↓
SnapshotCache
```

Current implemented runtime diagnostics chains:

```text
BasicHttpClient
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics

VdrSnapshotBuilder
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics

PollingService
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics

RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics
    ↓
RuntimeDiagnosticsJsonSerializer
    ↓
RuntimeDiagnosticsController
    ↓
GET /api/runtime

RuntimeDiagnosticsService
    ↓
RuntimeMeasurementSummary
    ↓
count
minDurationMs
maxDurationMs
lastDurationMs
lastStatusCode
lastSizeBytes
```

Purpose:

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- keep refresh decisions inside daemon-owned services
- keep snapshot storage separate from polling logic
- preserve full snapshot rebuild behavior for first poll and recovery paths
- prepare future multi-VDR and permission-aware designs
- keep API controllers backend-independent
- expose runtime progress through optional logging instead of direct service-level console output
- expose HTTP request timing through structured runtime measurements
- expose snapshot-domain build timing through structured runtime measurements
- expose polling-cycle and refresh-path timing through structured runtime measurements
- keep logging and diagnostics separate
- avoid unbounded diagnostics measurement growth through service-owned retention
- provide grouped measurement summaries through a dedicated `/api/runtime/summary` endpoint without changing the public `/api/runtime` JSON format
- keep future third-party or multi-client API consumers possible through a platform-oriented API strategy

---

## Implemented Snapshot and Change-State Components

Implemented:

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
- `VdrOverviewService` snapshot-backed overview path
- `VdrController` snapshot-backed overview test coverage
- `IVdrAdapter::getChangeState()`
- `VdrService::getChangeState()`
- `MockVdrAdapter::getChangeState()`
- `ExternalVdrAdapter::getChangeState()`
- `RestfulApiVdrAdapter::getChangeState()`

Polling integration:

- `PollingService` reads `VdrService::getChangeState()` before refresh planning
- first poll builds an initial full snapshot
- unchanged change-state keeps the existing cached snapshot
- changed change-state is converted into `VdrChangeEvent` entries
- `SnapshotRefreshPlanner` converts change events into `SnapshotUpdatePlan` values
- generated update plans are executed domain-by-domain through `VdrSnapshotBuilder` and `SnapshotCacheService`
- `BasicHttpClient` can emit structured HTTP runtime measurements through `IRuntimeMeasurementSink`
- `VdrSnapshotBuilder` can emit structured snapshot-domain runtime measurements through `IRuntimeMeasurementSink`
- `PollingService` can emit structured polling and refresh-path runtime measurements through `IRuntimeMeasurementSink`
- `DaemonRuntime` wires `ConsoleRuntimeLogger` into runtime components
- `DaemonRuntime` owns `RuntimeDiagnosticsService` and wires it into current measurement producers
- `DaemonRuntime` owns `RuntimeDiagnosticsController` and exposes diagnostics through `GET /api/runtime`
