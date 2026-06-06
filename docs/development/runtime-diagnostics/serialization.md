# Runtime Diagnostics Serialization

## Overview

Runtime diagnostics JSON generation is implemented by:

```text
RuntimeDiagnosticsJsonSerializer
```

The serializer is responsible for converting:

- RuntimeMeasurement
- RuntimeMeasurementSummary
- RuntimeDiagnostics

into REST-compatible JSON.

## Measurement Serialization

Measurements expose:

- component
- operation
- durationMs
- itemCount
- statusCode
- sizeBytes
- timestamp

## Summary Serialization

Summaries expose:

- component
- operation
- count
- itemCount
- minDurationMs
- maxDurationMs
- lastDurationMs
- lastStatusCode
- lastSizeBytes

## Controller Integration

The serializer is used by:

```text
RuntimeDiagnosticsController
```

Endpoints:

```text
GET /api/runtime
GET /api/runtime/summary
```

## Testing

Primary coverage:

```text
test_runtime_diagnostics_json_serializer
```

Additional coverage:

```text
test_runtime_diagnostics_controller
test_runtime_item_count_serialization
```
