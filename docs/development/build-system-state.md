# VDR-Suite – Build System State

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

This document contains the build system state that was split out of `docs/development/current-status.md` during Phase 10.21.1.

---

## Build System State

The top-level `Makefile` delegates source groups to modular include files under `mk/`.

Implemented build modules include:

- `mk/common.mk`
- `mk/recording-sources.mk`
- `mk/action-job-sources.mk`
- `mk/rest-sources.mk`
- `mk/daemon-sources.mk`
- `mk/http-sources.mk`
- `mk/runtime-sources.mk`
- `mk/vdr-sources.mk`
- `mk/vdr-tests.mk`

The public targets remain stable:

- `make test`
- `make clean`
- `make daemon`
- `make dashboard-cli`
- `make prepare-test-db`

Runtime diagnostics test state:

- `make test-runtime-diagnostics` validates the internal `RuntimeDiagnostics` and `RuntimeMeasurement` model.
- `test_runtime_diagnostics_service` validates `RuntimeDiagnosticsService`, `IRuntimeMeasurementSink` usage, retention behavior and internal measurement summaries.
- `test_vdr_snapshot_builder` validates `VdrSnapshotBuilder` measurement-sink recording.
- `test_polling_service` validates `PollingService` measurement-sink recording while preserving existing polling behavior.
- `test_runtime_diagnostics_json_serializer` validates diagnostics JSON serialization.
- `test_runtime_diagnostics_controller` validates the read-only REST controller response.
- `test_api_router` validates `/api/runtime` and `/api/runtime/summary` routing.
- `test_test_http_server` validates `/api/runtime` and `/api/runtime/summary` through the HTTP server layer.
- `make test` includes the runtime diagnostics, serializer, summary endpoint, controller, router, HTTP server, snapshot-builder and polling-service measurement tests.

Local VDR integration targets are intentionally optional and are not part of `make test`:

- `make test-local-restfulapi-integration`
- `make test-local-snapshot-runtime-integration`
- `make test-local-partial-refresh-validation`
---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
