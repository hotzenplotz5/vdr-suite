# Phase 10 Runtime Diagnostics Measurement Collection

## Purpose

This document records the current state of the Phase 10 runtime diagnostics work after the snapshot builder measurement collection step.

It complements `docs/development/current-status.md` while the runtime diagnostics block is still in progress.

---

## Verified State

The following steps are implemented and locally verified:

- Phase 10.10: `IRuntimeMeasurementSink`
- Phase 10.11: `RuntimeDiagnosticsService` implements `IRuntimeMeasurementSink`
- Phase 10.12: `BasicHttpClient` records HTTP runtime measurements
- Phase 10.13: `VdrSnapshotBuilder` records snapshot-domain runtime measurements

Local verification completed with:

```text
make test-vdr-snapshot-builder
make daemon
make test
git status
```

The repository was clean and synchronized with `origin/phase-2-actions` after the local verification run.

---

## Runtime Measurement Chains

Implemented HTTP measurement chain:

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
```

Implemented snapshot-builder measurement chain:

```text
VdrSnapshotBuilder
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics
```

This keeps logging and diagnostics separate.

Runtime logging still writes human-readable log entries through `IRuntimeLogger`.

Runtime diagnostics receive structured `RuntimeMeasurement` values through `IRuntimeMeasurementSink`.

No log text parsing is required.

---

## Implemented Runtime Diagnostics Components

Current runtime diagnostics components:

- `RuntimeMeasurement`
- `RuntimeDiagnostics`
- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`
- `test_runtime_diagnostics`
- `test_runtime_diagnostics_service`
- `test_vdr_snapshot_builder` measurement-sink coverage

`RuntimeDiagnosticsService` owns an internal `RuntimeDiagnostics` instance and implements:

- `recordMeasurement(...)`
- `diagnostics()`
- `empty()`
- `size()`
- `clear()`

---

## BasicHttpClient Measurement Collection

`BasicHttpClient` supports two optional runtime observers:

```text
IRuntimeLogger* logger
IRuntimeMeasurementSink* measurementSink
```

The HTTP client still logs timing information when a logger is provided.

In addition, after a successful HTTP request it records a structured measurement containing:

- component: `BasicHttpClient`
- operation: request method and URL
- duration in milliseconds
- HTTP status code
- response body size in bytes

This provides the first real runtime-diagnostics data source.

---

## VdrSnapshotBuilder Measurement Collection

`VdrSnapshotBuilder` supports two optional runtime observers:

```text
IRuntimeLogger* logger
IRuntimeMeasurementSink* measurementSink
```

The snapshot builder still logs domain build timing information when a logger is provided.

In addition, it records structured measurements for:

- `Build status`
- `Build recordings`
- `Build timers`
- `Build channels`
- `Build events`

Each emitted measurement contains:

- component: `VdrSnapshotBuilder`
- operation: snapshot-domain build operation
- duration in milliseconds
- item count in `sizeBytes` for collection domains

The following collection domains record their returned item count in `sizeBytes`:

- recordings
- timers
- channels
- events

The status domain has no collection size and keeps the default measurement size.

---

## Test Coverage

Current measurement coverage includes:

- `test_runtime_diagnostics`
- `test_runtime_diagnostics_service`
- `test_mock_http_client`
- `test_vdr_snapshot_builder`

`test_vdr_snapshot_builder` now verifies that:

- a complete snapshot records five measurements
- status measurement is recorded
- recordings measurement is recorded
- timers measurement is recorded
- channels measurement is recorded
- events measurement is recorded
- collection-domain item counts are captured in `sizeBytes`

---

## What Is Not Implemented Yet

The following are intentionally not implemented yet:

- daemon-owned `RuntimeDiagnosticsService` wiring
- `PollingService` measurement collection
- diagnostics JSON serialization
- diagnostics REST controller
- `/api/runtime` endpoint
- persistence of runtime measurements
- rolling windows, averages or aggregation policies

---

## Architectural Rule

Diagnostics must not be derived by parsing human-readable log text.

Correct direction:

```text
runtime component
    ↓
creates RuntimeMeasurement directly
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
```

Avoid:

```text
ConsoleRuntimeLogger
    ↓
parse log text
    ↓
RuntimeMeasurement
```

This preserves a clean boundary between logging and diagnostics.

---

## Next Steps

Recommended next phases:

```text
Phase 10.14: PollingService runtime measurements
Phase 10.15: daemon-owned RuntimeDiagnosticsService wiring
Phase 10.16: RuntimeDiagnostics JSON serializer
Phase 10.17: Runtime diagnostics REST endpoint
```

The REST endpoint should only be added after HTTP, snapshot and polling measurements are collected in the daemon runtime.
