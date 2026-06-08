# Runtime Diagnostics Test Coverage

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

## Core Diagnostics

Covered by:

- test_runtime_diagnostics
- test_runtime_diagnostics_service
- test_runtime_diagnostics_json_serializer
- test_runtime_diagnostics_controller

Verified behaviour:

- measurement recording
- retention limits
- summary aggregation
- JSON serialization
- controller responses

## Snapshot Diagnostics

Covered by:

- test_vdr_snapshot_builder

Verified behaviour:

- status measurement recording
- recordings measurement recording
- timers measurement recording
- channels measurement recording
- events measurement recording
- item count propagation

## Polling Diagnostics

Covered by:

- test_polling_service

Verified behaviour:

- initial poll measurements
- refresh measurements
- partial refresh measurements
- poll cycle measurements

## REST Coverage

Covered by:

- test_api_router
- test_test_http_server

Verified behaviour:

- /api/runtime routing
- /api/runtime/summary routing
- HTTP exposure

## Full Validation

Historically verified through:

```text
make test
make daemon
```

All diagnostics functionality must continue to pass complete repository validation.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
