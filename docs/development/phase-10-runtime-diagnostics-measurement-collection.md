# Phase 10 Runtime Diagnostics Measurement Collection

## Purpose

This document records the current state of the Phase 10 runtime diagnostics work after the runtime diagnostics retention, measurement summary and public summary endpoint steps.

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
- Phase 10.16: `RuntimeDiagnosticsController` exposes collected diagnostics through `GET /api/runtime`
- Phase 10.17: `RuntimeDiagnosticsService` applies bounded in-memory retention to collected measurements
- Phase 10.18: `RuntimeDiagnosticsService` exposes internal measurement summaries grouped by component and operation
- Phase 10.19: `RuntimeDiagnosticsController` exposes summaries through `GET /api/runtime/summary`

Local verification was reported with successful visible output for:

```text
make test-runtime-diagnostics-controller
make test-api-router
make test-test-http-server
make test
make daemon
git status
```

The local result showed successful targeted runtime diagnostics serializer, controller, router and HTTP server tests, successful full `make test`, successful `make daemon` and a clean synchronized working tree after the Phase 10.19 implementation.

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

Implemented REST exposure chain:

```text
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics
    ↓
RuntimeDiagnosticsJsonSerializer
    ↓
RuntimeDiagnosticsController
    ↓
GET /api/runtime
```

Implemented public summary chain:

```text
RuntimeDiagnosticsService
    ↓
RuntimeMeasurementSummary
    ↓
RuntimeDiagnosticsJsonSerializer
    ↓
RuntimeDiagnosticsController
    ↓
GET /api/runtime/summary
    ↓
count
minDurationMs
maxDurationMs
lastDurationMs
lastStatusCode
lastSizeBytes
```

The summary endpoint exposes grouped diagnostics data without changing the raw measurements endpoint.

This keeps logging and diagnostics separate.

Runtime logging still writes human-readable log entries through `IRuntimeLogger`.

Runtime diagnostics receive structured `RuntimeMeasurement` values through `IRuntimeMeasurementSink`.

`RuntimeDiagnosticsJsonSerializer` converts the collected diagnostics into JSON.

`RuntimeDiagnosticsController` exposes raw retained diagnostics through `GET /api/runtime` and grouped summaries through `GET /api/runtime/summary`.

No log text parsing is required.

---

## Implemented Runtime Diagnostics Components

Current runtime diagnostics components:

- `RuntimeMeasurement`
- `RuntimeDiagnostics`
- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`
- `RuntimeMeasurementSummary`
- `RuntimeDiagnosticsJsonSerializer`
- `RuntimeDiagnosticsController`
- `GET /api/runtime`
- `GET /api/runtime/summary`
- `test_runtime_diagnostics`
- `test_runtime_diagnostics_service`
- `test_runtime_diagnostics_json_serializer`
- `test_runtime_diagnostics_controller`
- `test_vdr_snapshot_builder` measurement-sink coverage
- `test_polling_service` measurement-sink coverage
- `test_api_router` `/api/runtime` and `/api/runtime/summary` routing coverage
- `test_test_http_server` `/api/runtime` and `/api/runtime/summary` HTTP server coverage

`RuntimeDiagnosticsService` owns an internal `RuntimeDiagnostics` instance, applies bounded in-memory retention and implements:

- `recordMeasurement(...)`
- `diagnostics()`
- `empty()`
- `size()`
- `clear()`
- `measurementSummaries()`

---

## Runtime Diagnostics Retention and Summaries

`RuntimeDiagnosticsService` now keeps collected measurements bounded in memory.

The default retention limit is:

```text
100 measurements
```

When the configured limit is exceeded, the oldest measurements are removed first.

This keeps diagnostics memory use bounded while preserving the newest measurements for `/api/runtime`.

`RuntimeDiagnosticsService::measurementSummaries()` provides an internal grouped summary view over the retained measurements.

Summaries are grouped by:

- component
- operation

Each `RuntimeMeasurementSummary` contains:

- component
- operation
- count
- minDurationMs
- maxDurationMs
- lastDurationMs
- lastStatusCode
- lastSizeBytes

The summary view is exposed through the dedicated `GET /api/runtime/summary` endpoint.
It does not change the public `/api/runtime` JSON response.
This preserves the existing raw-measurement REST contract while providing an explicit diagnostics summary API.

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

`DaemonRuntime` also owns the REST diagnostics view:

```text
RuntimeDiagnosticsJsonSerializer
RuntimeDiagnosticsController
```

This means the daemon-owned runtime diagnostics service can collect structured measurements from HTTP access, snapshot building and polling execution and expose the current diagnostics state through `/api/runtime` without parsing log output.

---

## Test Coverage

Current measurement and endpoint coverage includes:

- `test_runtime_diagnostics`
- `test_runtime_diagnostics_service`
- `test_runtime_diagnostics_json_serializer`
- `test_runtime_diagnostics_controller`
- `test_mock_http_client`
- `test_vdr_snapshot_builder`
- `test_polling_service`
- `test_api_router`
- `test_test_http_server`

`test_runtime_diagnostics_service` verifies that:

- measurements are recorded through the diagnostics service
- retained measurement order is preserved
- bounded retention removes oldest measurements first
- internal measurement summaries are grouped by component and operation
- summary count, minimum duration, maximum duration and last values are calculated from retained measurements

`test_runtime_diagnostics_json_serializer` verifies that:

- empty diagnostics serialize as an empty `measurements` array
- recorded measurements are serialized with component, operation, duration, status code and size
- strings are JSON-escaped

`test_runtime_diagnostics_controller` verifies that:

- the controller returns HTTP status `200`
- the response content type is `application/json`
- empty diagnostics are exposed as `{"measurements":[]}`
- recorded measurements are exposed through the existing diagnostics JSON serializer

`test_api_router` verifies that `/api/runtime` is routed to the runtime diagnostics controller.

`test_test_http_server` verifies that `/api/runtime` is reachable through the HTTP server layer.

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

- persistence of runtime measurements
- public summary JSON or rolling averages
- runtime diagnostics API hardening beyond the initial read-only endpoint

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

`GET /api/runtime` must remain a diagnostics endpoint. It must not become a general debug dump and it must not expose backend-specific VDR state.

---

## Next Steps

Recommended next phases:

```text
Phase 10.19: Runtime diagnostics API summary exposure decision
Phase 10.20: Runtime diagnostics API hardening or integration validation
```

The REST endpoint remains a read-only view on the existing daemon-owned runtime diagnostics measurement state. Internal summaries are available in the service layer but are not yet exposed through a public JSON contract.
