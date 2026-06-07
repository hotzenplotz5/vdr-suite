# VDR-Suite – Runtime Diagnostics Status

This document contains the Phase 10 runtime logging and diagnostics state that was split out of `docs/development/current-status.md` during Phase 10.21.1.

---

## Phase 10 Runtime Logging and Diagnostics State

Phase 10 introduced runtime logging first and then started structured runtime diagnostics.

Implemented runtime logging components:

- `RuntimeLogLevel`
- `RuntimeLogEntry`
- `IRuntimeLogger`
- `NullRuntimeLogger`
- `ConsoleRuntimeLogger`
- runtime log level formatting
- optional `PollingService` runtime logging
- optional `VdrService` runtime logging
- optional `VdrSnapshotBuilder` domain timing logs
- optional `BasicHttpClient` request timing logs
- `ConsoleRuntimeLogger` wiring inside `DaemonRuntime`

Implemented runtime diagnostics components:

- `RuntimeMeasurement`
- `RuntimeDiagnostics`
- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`
- `RuntimeMeasurementSummary`
- daemon-owned `RuntimeDiagnosticsService` wiring
- `test_runtime_diagnostics`
- `test_runtime_diagnostics_service`
- `BasicHttpClient` structured HTTP measurement recording
- `VdrSnapshotBuilder` structured snapshot-domain measurement recording
- `PollingService` structured poll-cycle and refresh-path measurement recording
- `RuntimeDiagnosticsJsonSerializer`
- `RuntimeDiagnosticsController`
- `GET /api/runtime`
- runtime diagnostics retention policy
- internal runtime measurement summaries
- `GET /api/runtime/summary`
- `test_vdr_snapshot_builder` measurement-sink coverage
- `test_polling_service` measurement-sink coverage
- `test_runtime_diagnostics_json_serializer`
- `test_runtime_diagnostics_controller`
- `test_api_router` `/api/runtime` and `/api/runtime/summary` routing coverage
- `test_test_http_server` `/api/runtime` and `/api/runtime/summary` HTTP server coverage

Current diagnostics design:

- diagnostics do not parse human-readable log output
- runtime components create `RuntimeMeasurement` values directly
- `IRuntimeMeasurementSink` decouples producers from diagnostics storage
- `RuntimeDiagnosticsService` stores measurements in `RuntimeDiagnostics`
- `RuntimeDiagnosticsService` keeps retained measurements bounded in memory
- `RuntimeDiagnosticsService` can calculate internal measurement summaries grouped by component and operation
- `BasicHttpClient` records component, operation, duration, status code and body size for successful requests
- `VdrSnapshotBuilder` records component, operation, duration and domain item count for snapshot-domain build operations
- `PollingService` records component, operation, duration and selected counts for poll-cycle and refresh-path work
- `DaemonRuntime` owns one `RuntimeDiagnosticsService` and passes it into `BasicHttpClient`, `VdrSnapshotBuilder` and `PollingService`
- `RuntimeDiagnosticsJsonSerializer` serializes collected measurements as JSON
- `RuntimeDiagnosticsController` exposes raw diagnostics through `GET /api/runtime` and grouped summaries through `GET /api/runtime/summary`

Not implemented yet:

- rolling averages
- runtime diagnostics persistence

---

## Runtime Diagnostics Test State

- `make test-runtime-diagnostics` validates the internal `RuntimeDiagnostics` and `RuntimeMeasurement` model.
- `test_runtime_diagnostics_service` validates `RuntimeDiagnosticsService`, `IRuntimeMeasurementSink` usage, retention behavior and internal measurement summaries.
- `test_vdr_snapshot_builder` validates `VdrSnapshotBuilder` measurement-sink recording.
- `test_polling_service` validates `PollingService` measurement-sink recording while preserving existing polling behavior.
- `test_runtime_diagnostics_json_serializer` validates diagnostics JSON serialization.
- `test_runtime_diagnostics_controller` validates the read-only REST controller response.
- `test_api_router` validates `/api/runtime` and `/api/runtime/summary` routing.
- `test_test_http_server` validates `/api/runtime` and `/api/runtime/summary` through the HTTP server layer.
- `make test` includes the runtime diagnostics, serializer, summary endpoint, controller, router, HTTP server, snapshot-builder and polling-service measurement tests.
