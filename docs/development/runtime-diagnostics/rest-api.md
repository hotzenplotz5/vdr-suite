# Runtime Diagnostics REST API

## Overview

Runtime diagnostics are exposed through dedicated read-only REST endpoints.

The endpoints expose structured runtime measurements and aggregated summaries.

Runtime diagnostics are not derived from log parsing.

## Raw Measurements

Endpoint:

```text
GET /api/runtime
```

Pipeline:

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

The endpoint exposes retained runtime measurements.

## Measurement Summaries

Endpoint:

```text
GET /api/runtime/summary
```

Pipeline:

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

Summary values include:

- component
- operation
- count
- itemCount
- minDurationMs
- maxDurationMs
- lastDurationMs
- lastStatusCode
- lastSizeBytes

Summaries are grouped by:

- component
- operation

## Architectural Rule

Runtime diagnostics endpoints must remain diagnostics APIs.

They must not become:

- debug dumps
- backend state exports
- VDR internal object views

Diagnostics data must originate from structured RuntimeMeasurement objects.

Log parsing is explicitly forbidden.
