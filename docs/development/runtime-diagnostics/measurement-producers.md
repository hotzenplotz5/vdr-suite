# Runtime Diagnostics Measurement Producers

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

Runtime diagnostics measurements are produced by runtime components that already perform work in the daemon path.

All producers send `RuntimeMeasurement` values through `IRuntimeMeasurementSink`.

## BasicHttpClient

`BasicHttpClient` supports two optional observers:

```text
IRuntimeLogger* logger
IRuntimeMeasurementSink* measurementSink
```

For successful HTTP requests it records:

- component: `BasicHttpClient`
- operation: request method and URL
- duration in milliseconds
- HTTP status code
- response body size in bytes through `sizeBytes`

This provides the HTTP runtime diagnostics data source.

## VdrSnapshotBuilder

`VdrSnapshotBuilder` supports two optional observers:

```text
IRuntimeLogger* logger
IRuntimeMeasurementSink* measurementSink
```

It records structured measurements for:

- `Build status`
- `Build recordings`
- `Build timers`
- `Build channels`
- `Build events`

Each emitted measurement contains:

- component: `VdrSnapshotBuilder`
- operation: snapshot-domain build operation
- duration in milliseconds
- collection-domain item count through `itemCount`

The following collection domains record returned item counts through `itemCount`:

- recordings
- timers
- channels
- events

The status domain has no collection item count and keeps the default item count value.

## PollingService

`PollingService` supports two optional observers:

```text
IRuntimeLogger* logger
IRuntimeMeasurementSink* measurementSink
```

The polling service preserves the existing polling behavior:

- first poll builds a complete initial snapshot
- unchanged change-state does not refresh cached domains
- changed change-state is converted into `VdrChangeEvent` entries
- `SnapshotRefreshPlanner` creates a `SnapshotUpdatePlan`
- required refresh work is executed through `VdrSnapshotBuilder` and `SnapshotCacheService`

It records structured measurements for polling and refresh paths:

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

Polling measurements use:

- component: `PollingService`
- operation: polling or refresh operation
- duration in milliseconds
- item counts through `itemCount` where a count is recorded

Phase 10.20 moved count-like values away from `sizeBytes` into `itemCount`.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
