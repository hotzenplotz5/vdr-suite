# Runtime Diagnostics Daemon Integration

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

## Purpose

Runtime diagnostics are collected during daemon execution.

The daemon is the primary producer of runtime measurement data.

## Integration Path

```text
DaemonRuntime
    ↓
PollingService
    ↓
VdrSnapshotBuilder
    ↓
RuntimeDiagnosticsService
```

## Recorded Operations

Examples:

- poll cycle execution
- snapshot refreshes
- channel retrieval
- event retrieval
- timer retrieval
- recording retrieval

## Diagnostics Exposure

Collected measurements become available through:

```text
GET /api/runtime
GET /api/runtime/summary
```

without requiring daemon restart.

## Architectural Rule

Daemon components must never serialize diagnostics directly.

All diagnostics access must flow through RuntimeDiagnosticsService and RuntimeDiagnosticsController.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
