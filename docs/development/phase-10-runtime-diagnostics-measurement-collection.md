# Phase 10 Runtime Diagnostics Measurement Collection

## Purpose

This document records the current state of the Phase 10 runtime diagnostics work after the PollingService measurement collection and daemon diagnostics wiring step.

It complements `docs/development/current-status.md` while the runtime diagnostics block is still in progress.

---

## Verified State

The following steps are implemented:

- Phase 10.10: `IRuntimeMeasurementSink`
- Phase 10.11: `RuntimeDiagnosticsService` implements `IRuntimeMeasurementSink`
- Phase 10.12: `BasicHttpClient` records HTTP runtime measurements
- Phase 10.13: `VdrSnapshotBuilder` records snapshot-domain runtime measurements
- Phase 10.14: `PollingService` records polling-cycle and refresh-path runtime measurements
- Phase 10.14: `DaemonRuntime` owns `RuntimeDiagnosticsService` and wires it into current measurement producers
- Phase 10.15: `RuntimeDiagnosticsJsonSerializer` serializes collected diagnostics measurements

Local verification was reported with successful visible output for:

```text
make test-polling-service
make test-vdr-snapshot-builder
make daemon
```

The submitted `make test` output was truncated in the chat transcript before the final `git status` result was visible. Full repository cleanliness should therefore be confirmed locally before tagging this phase.

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

Implemented polling measurement chain:

```text
PollingService
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
- `test_polling_service` measurement-sink coverage

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

This provides the HTTP runtime-diagnostics data source.

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

## PollingService Measurement Collection

`PollingService` now supports two optional runtime observers:

```text
IRuntimeLogger* logger
IRuntimeMeasurementSink* measurementSink
```

The polling service still preserves existing polling behavior:

- first poll builds a complete initial snapshot
- unchanged change-state does not refresh cached domains
- changed change-state is converted into `VdrChangeEvent` entries
- `SnapshotRefreshPlanner` creates the `SnapshotUpdatePlan`
- required refresh work is executed through `VdrSnapshotBuilder` and `SnapshotCacheService`

In addition, it records structured measurements for polling and refresh paths:

- `Poll cycle`
- `Initial snapshot poll`
- `Detect changes`
- `Create update plan`
- `Full snapshot refresh`
- `Partial refresh`
- `Status refresh`
- `Recordings refresh`
- `Timers refresh`
- `Channels refresh`
- `Events refresh`

The polling measurements use:

- component: `PollingService`
- operation: polling or refresh operation
- duration in milliseconds
- change-event count in `sizeBytes` for change detection and update-plan creation

---

## Daemon Runtime Diagnostics Wiring

`DaemonRuntime` now owns a `RuntimeDiagnosticsService` instance.

The same diagnostics service is passed as `IRuntimeMeasurementSink` into:

- `BasicHttpClient`
- `VdrSnapshotBuilder`
- `PollingService`

This means the daemon-owned runtime diagnostics service can collect structured measurements from HTTP access, snapshot building and polling execution without parsing log output.

---

## Test Coverage

Current measurement coverage includes:

- `test_runtime_diagnostics`
- `test_runtime_diagnostics_service`
- `test_mock_http_client`
- `test_vdr_snapshot_builder`
- `test_polling_service`

`test_vdr_snapshot_builder` verifies that:

- a complete snapshot records five measurements
- status measurement is recorded
- recordings measurement is recorded
- timers measurement is recorded
- channels measurement is recorded
- events measurement is recorded
- collection-domain item counts are captured in `sizeBytes`

`test_polling_service` verifies that existing polling behavior is preserved and that polling measurements are recorded for:

- first poll
- unchanged poll detection and plan creation
- channel refresh
- recordings refresh
- partial refresh
- poll cycle

---

## What Is Not Implemented Yet

The following are intentionally not implemented yet:

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
Phase 10.16: Runtime diagnostics REST endpoint
Phase 10.17: Runtime diagnostics aggregation policy
```

The REST endpoint should only be added after collection and serialization are stable.
