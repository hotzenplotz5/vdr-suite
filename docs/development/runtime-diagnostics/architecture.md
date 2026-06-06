# Runtime Diagnostics Architecture

Runtime diagnostics are separate from runtime logging.

Human-readable runtime logging is written through `IRuntimeLogger` and is intended for operators and developers reading daemon output.

Structured runtime diagnostics are created as `RuntimeMeasurement` values and passed through `IRuntimeMeasurementSink`. Diagnostics must not be derived by parsing log text.

## Implemented chains

HTTP measurement chain:

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

Snapshot-builder measurement chain:

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

Polling measurement chain:

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

REST exposure chain:

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

Summary exposure chain:

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
```

## Design rules

- Runtime diagnostics must not parse `ConsoleRuntimeLogger` output.
- Runtime components create `RuntimeMeasurement` directly.
- Producers depend only on `IRuntimeMeasurementSink`.
- `RuntimeDiagnosticsService` owns storage and summary calculation.
- REST controllers expose diagnostics, but do not collect measurements.
- `GET /api/runtime` remains a diagnostics endpoint, not a general debug dump.
- Backend-specific VDR state must not leak into runtime diagnostics endpoints.
