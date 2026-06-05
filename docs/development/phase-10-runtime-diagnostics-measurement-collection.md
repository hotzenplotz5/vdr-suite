# Phase 10 Runtime Diagnostics Measurement Collection

## Purpose

This document records the current state of the Phase 10 runtime diagnostics work after the HTTP measurement collection step.

It complements `docs/development/current-status.md` and avoids rewriting the large status file while the runtime diagnostics block is still in progress.

---

## Verified State

The following steps are implemented and locally verified:

- Phase 10.10: `IRuntimeMeasurementSink`
- Phase 10.11: `RuntimeDiagnosticsService` implements `IRuntimeMeasurementSink`
- Phase 10.12: `BasicHttpClient` records HTTP runtime measurements

Local verification completed with:

```text
make test-mock-http-client
make daemon
make test
git status
```

The repository was clean and synchronized with `origin/phase-2-actions` after the local verification run.

---

## Runtime Measurement Chain

Implemented measurement chain:

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

`RuntimeDiagnosticsService` owns an internal `RuntimeDiagnostics` instance and implements:

- `recordMeasurement(...)`
- `diagnostics()`
- `empty()`
- `size()`
- `clear()`

---

## BasicHttpClient Measurement Collection

`BasicHttpClient` now supports two optional runtime observers:

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

## What Is Not Implemented Yet

The following are intentionally not implemented yet:

- daemon-owned `RuntimeDiagnosticsService` wiring
- `VdrSnapshotBuilder` measurement collection
- `PollingService` measurement collection
- diagnostics JSON serialization
- diagnostics REST controller
- `/api/runtime` endpoint
- persistence of runtime measurements
- rolling windows, averages or aggregation policies

---

## Attempted Phase 10.13 Direction

The next planned implementation step is:

```text
VdrSnapshotBuilder
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
```

The existing `VdrSnapshotBuilder` already measures and logs domain build durations for:

- status
- recordings
- timers
- channels
- events

The intended next step is to pass an optional measurement sink into `VdrSnapshotBuilder` and emit structured measurements for these domain build operations.

At the time this document was written, GitHub write operations for the VDR snapshot builder files were blocked by the execution environment before reaching GitHub. Therefore Phase 10.13 was not applied.

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
Phase 10.13: VdrSnapshotBuilder runtime measurements
Phase 10.14: PollingService runtime measurements
Phase 10.15: daemon-owned RuntimeDiagnosticsService wiring
Phase 10.16: RuntimeDiagnostics JSON serializer
Phase 10.17: Runtime diagnostics REST endpoint
```

The REST endpoint should only be added after at least HTTP and snapshot measurements are collected in the daemon runtime.
